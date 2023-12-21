#include "matoya.h"

#include <stdio.h>

static void *output_hex(const char *prefix, size_t index, uint8_t *buf, size_t size)
{
	size_t out_size = 1024;
	size_t o = 0;

	char *out = MTY_Alloc(out_size + 8, 1);
	o += snprintf(out, out_size - o, "static const unsigned char %s_%zu[] = {", prefix, index);

	for (size_t x = 0; x < size; x++) {
		if (o + 6 >= out_size) {
			out_size += 1024;
			out = MTY_Realloc(out, out_size + 8, 1);
		}

		snprintf(out + o, out_size - o, "0x%02X,", buf[x]);
		o += 5;
	}

	snprintf(out + o - 1, out_size - o + 1, "};\n");

	return out;
}

int32_t main(int32_t argc, char **argv)
{
	if (argc < 4)
		return 1;

	const char *fname = argv[1];
	const char *prefix = argv[2];
	const char *src = argv[3];

	MTY_FileList *list = MTY_GetFileList(src, NULL);
	if (!list)
		return 1;

	size_t num_files = 0;

	MTY_DeleteFile(fname);

	for (uint32_t x = 0; x < list->len; x++) {
		MTY_FileDesc *desc = &list->files[x];

		if (!desc->dir) {
			const char *path = MTY_JoinPath(src, desc->name);

			size_t inSize = 0;
			void *in = MTY_ReadFile(path, &inSize);

			if (in) {
				size_t outSize = 0;
				void *out = MTY_Compress(in, inSize, &outSize);

				if (out) {
					char *hex = output_hex(prefix, num_files, out, outSize);
					MTY_AppendTextToFile(fname, "%s", hex);
					MTY_AppendTextToFile(fname, "static const char %s_%zu_NAME[] = \"%s\";\n",
						prefix, num_files, desc->name);
					MTY_Free(hex);
					MTY_Free(out);

					num_files++;
				}

				MTY_Free(in);
			}
		}
	}

	MTY_AppendTextToFile(fname, "\n#define %s_LEN %zu\n\n", prefix, num_files);

	MTY_AppendTextToFile(fname,
		"static const struct {\n"
		"\tsize_t size;\n"
		"\tconst char *name;\n"
		"\tconst unsigned char *buf;\n"
		"} %s[%s_LEN] = {\n", prefix, prefix);

	for (size_t x = 0; x < num_files; x++) {
		MTY_AppendTextToFile(fname,
		"\t{sizeof(%s_%zu), %s_%zu_NAME, %s_%zu},\n", prefix, x, prefix, x, prefix, x);
	}

	MTY_AppendTextToFile(fname, "};\n");

	MTY_FreeFileList(&list);

	return 0;
}
