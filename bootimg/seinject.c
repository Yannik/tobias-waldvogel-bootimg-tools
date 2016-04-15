/* 
 * This was derived from public domain works with updates to 
 * work with more modern SELinux libraries. 
 * 
 * It is released into the public domain.
 * 
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sepol/policydb/policydb.h>
#include <sepol/policydb/services.h>

#ifdef WIN32
#define strtok_r strtok_s
#endif

void *cmalloc(size_t s) {
	void *t = malloc(s);
	if (t == NULL) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	return t;
}

int add_rule(char *s, char *t, char *c, char *p, policydb_t *policy) {
	type_datum_t *src, *tgt;
	class_datum_t *cls;
	perm_datum_t *perm;
	avtab_datum_t *av;
	avtab_key_t key;

	src = hashtab_search(policy->p_types.table, s);
	if (src == NULL) {
		fprintf(stderr, "source type %s does not exist\n", s);
		return 2;
	}
	tgt = hashtab_search(policy->p_types.table, t);
	if (tgt == NULL) {
		fprintf(stderr, "target type %s does not exist\n", t);
		return 2;
	}
	cls = hashtab_search(policy->p_classes.table, c);
	if (cls == NULL) {
		fprintf(stderr, "class %s does not exist\n", c);
		return 2;
	}
	perm = hashtab_search(cls->permissions.table, p);
	if (perm == NULL) {
		if (cls->comdatum == NULL) {
			fprintf(stderr, "perm %s does not exist in class %s\n", p, c);
			return 2;
		}
		perm = hashtab_search(cls->comdatum->permissions.table, p);
		if (perm == NULL) {
			fprintf(stderr, "perm %s does not exist in class %s\n", p, c);
			return 2;
		}
	}

	// See if there is already a rule
	key.source_type = src->s.value;
	key.target_type = tgt->s.value;
	key.target_class = cls->s.value;
	key.specified = AVTAB_ALLOWED;
	av = avtab_search(&policy->te_avtab, &key);

	if (av == NULL) {
		int ret;

		av = cmalloc(sizeof av);
		av->data |= 1U << (perm->s.value - 1);
		ret = avtab_insert(&policy->te_avtab, &key, av);
		if (ret) {
			fprintf(stderr, "Error inserting into avtab\n");
			return 1;
		}	
	}

	av->data |= 1U << (perm->s.value - 1);

	return 0;
}

int load_policy(char *filename, policydb_t *policydb, struct policy_file *pf) {
	FILE*	f;
	size_t	size;
	void *data;
	int ret;

	f = fopen(filename, "rb");
	if (f == NULL) {
		fprintf(stderr, "Can't open '%s':  %s\n",
		        filename, strerror(errno));
		return 1;
	}

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);

	data = malloc(size);
	if (data == NULL) {
		fprintf(stderr, "Can't allocat memory\n");
		fclose(f);
		return 1;
	}

	if (fread(data, 1, size, f) != size) {
		fprintf(stderr, "Can't read policy file '%s':  %s\n", filename, strerror(errno));
		free(data);
		fclose(f);
		return 1;
	}

	policy_file_init(pf);
	pf->type = PF_USE_MEMORY;
	pf->data = (char*)data;
	pf->len = size;
	if (policydb_init(policydb)) {
		fprintf(stderr, "policydb_init: Out of memory!\n");
		free(data);
		fclose(f);
		return 1;
	}

	ret = policydb_read(policydb, pf, 1);
	if (ret) {
		fprintf(stderr, "error(s) encountered while parsing configuration\n");
		free(data);
		fclose(f);
		return 1;
	}

	free(data);
	fclose(f);
	return 0;
}

int load_policy_into_kernel(policydb_t *policydb) {
	FILE	*f;
	char *filename = "/sys/fs/selinux/load";
	int ret;
	void *data = NULL;
	size_t len;

	policydb_to_image(NULL, policydb, &data, &len);

	// based on libselinux security_load_policy()
	f = fopen(filename, "wb");
	if (f == NULL) {
		fprintf(stderr, "Can't open '%s':  %s\n", filename, strerror(errno));
		return 1;
	}

	ret = fwrite(data, 1, len, f);
	fclose(f);

	if (ret < 0) {
		fprintf(stderr, "Could not write policy to %s\n", filename);
		return 1;
	}
	return 0;
}

int main_seinject(int argc, char **argv) {
	char *policy = NULL, *source = NULL, *target = NULL, *clazz = NULL, *perm = NULL, *perm_token = NULL, *perm_saveptr = NULL, *outfile = NULL, *permissive = NULL;
	policydb_t policydb;
	struct policy_file pf, outpf;
	sidtab_t sidtab;
	int ret_add_rule;
	int load = 0;
	int quiet = 0;
	FILE *fp;
	int i;

	for (i=1; i<argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 's') {
				i++;
				source = argv[i];
				continue;
			}
			if (argv[i][1] == 't') {
				i++;
				target = argv[i];
				continue;
			}
			if (argv[i][1] == 'c') {
				i++;
				clazz = argv[i];
				continue;
			}
			if (argv[i][1] == 'p') {
				i++;
				perm = argv[i];
				continue;
			}
			if (argv[i][1] == 'P') {
				i++;
				policy = argv[i];
				continue;
			}
			if (argv[i][1] == 'o') {
				i++;
				outfile = argv[i];
				continue;
			}
			if (argv[i][1] == 'Z') {
				i++;
				permissive = argv[i];
				continue;
			}
			if (argv[i][1] == 'l') {
				load = 1;
				continue;
			}
			if (argv[i][1] == 'q') {
				quiet = 1;
				continue;
			}
			break;
		}
	}

	if (i < argc || argc == 1 || ((!source || !target || !clazz || !perm) && !permissive)) {
		fprintf(stderr, "%s -s <source type> -t <target type> -c <class> -p <perm>[,<perm2>,<perm3>,...] [-P <policy file>] [-o <output file>] [-l|--load]\n", argv[0]);
		fprintf(stderr, "%s -Z permissive_type [-P <policy file>] [-o <output file>] [-l|--load]\n", argv[0]);
		exit(1);
	}

	if (!policy)
		policy = "/sys/fs/selinux/policy";

	sepol_set_policydb(&policydb);
	sepol_set_sidtab(&sidtab);

	if (load_policy(policy, &policydb, &pf)) {
		fprintf(stderr, "Could not load policy\n");
		return 1;
	}

	if (policydb_load_isids(&policydb, &sidtab))
		return 1;

	if (permissive) {
		type_datum_t *type;
		type = hashtab_search(policydb.p_types.table, permissive);
		if (type == NULL) {
			fprintf(stderr, "type %s does not exist\n", permissive);
			return 2;
		}
		if (ebitmap_set_bit(&policydb.permissive_map, type->s.value, 1)) {
			fprintf(stderr, "Could not set bit in permissive map\n");
			return 1;
		}
	} else {
		perm_token = strtok_r(perm, ",", &perm_saveptr);
		while (perm_token) {
			ret_add_rule = add_rule(source, target, clazz, perm_token, &policydb);
			if (ret_add_rule) {
				fprintf(stderr, "Could not add rule for perm: %s\n", perm_token);
				return ret_add_rule;
			}
			perm_token = strtok_r(NULL, ",", &perm_saveptr);
		}
	}

	if (outfile) {
		fp = fopen(outfile, "wb");
		if (!fp) {
			fprintf(stderr, "Could not open outfile\n");
			return 1;
		}

		policy_file_init(&outpf);
		outpf.type = PF_USE_STDIO;
		outpf.fp = fp;

		if (policydb_write(&policydb, &outpf)) {
			fprintf(stderr, "Could not write policy\n");
			return 1;
		}

		fclose(fp);
	}

	if (load) {
		if (load_policy_into_kernel(&policydb)) {
			fprintf(stderr, "Could not load new policy into kernel\n");
			return 1;
		}
	}

	policydb_destroy(&policydb);

	if (quiet == 0)
		fprintf(stdout, "Success\n");
	return 0;
}