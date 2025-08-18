/*
Usage:
```sh
memoize <command>
```
The above runs `<command>` and saves its `stdout` into `~/.memoize/history/<hash>.txt`
where the `<hash>` is generated from the given `<command>` and the current working directory.

Building:
```sh
gcc memoize.c -o memoize
```

Example usage with `fzf` with `fdfind`:
```sh
memoize fdfind --hidden | fzf
```
For this example to work install `fzf` and `fd-find` packages.

To override the `fzf` default behavior add this to your `.bashrc`
```sh
export FZF_DEFAULT_COMMAND='memoize fdfind --hidden'
export FZF_DEFAULT_OPTS="--bind 'ctrl-r:reload(memoize --reset fdfind --hidden)'"
```

or if you use `fish` to your `.config/fish/config.fish`
```sh
if status is-interactive
	set -x FZF_DEFAULT_COMMAND 'memoize fdfind --hidden'
	set -x FZF_DEFAULT_OPTS "--bind 'ctrl-r:reload(memoize --reset fdfind --hidden)'"
end
```

and then just run `fzf` normally to get the caching behavior. When the results get too stale, press
`cltr-r` to re-generate the cache for the current command and working directory.
*/

#include <assert.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int strict_snprintf(char *buffer, size_t size, const char *format, ...) {
	va_list args;
	va_start(args, format);
	int result = vsnprintf(buffer, size, format, args);
	va_end(args);

	//printf("%lld < %lld\n", (size_t)result, size);
	assert(result >= 0);
	assert((size_t)result < size);
	return result;
}

#define HASH_SIZE 32
#define BUFFER_SIZE 1024

unsigned int fnv1a_hash(const char *str, int hash) {

	const unsigned int FNV_PRIME = 16777619U;
	while (*str) {
		hash ^= (unsigned char)(*str++);
		hash *= FNV_PRIME;
	}
	return hash;
}

void hash_command(const char *command, char *hash_output) {
	char cwd[4096];
	if (!getcwd(cwd, sizeof(cwd)))
	{
		fprintf(stderr, "[ERROR] Failed to get current working directory");
	}

	const unsigned int FNV_OFFSET_BASIS = 2166136261U;
	unsigned int hash = FNV_OFFSET_BASIS;
	hash = fnv1a_hash(command, hash);
	hash = fnv1a_hash(cwd, hash);
	strict_snprintf(hash_output, HASH_SIZE + 1, "%08x", hash);
}

void execute_command(const char *command, const char *cache_file) {
	FILE *file = fopen(cache_file, "w");
	if (!file) {
		fprintf(stderr, "[ERROR] Failed to open cache file for writing '%s'", cache_file);
		return;
	}

	// Execute the command
	FILE *pipe = popen(command, "r");
	if (!pipe) {
		fprintf(stderr, "[ERROR] Failed to run '%s'", command);
		fclose(file);
		return;
	}

	char buffer[BUFFER_SIZE];
	while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
		fprintf(stdout, "%s", buffer);
		fputs(buffer, file);
	}

	pclose(pipe);
	fclose(file);
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "\
Usage: %s <command>\n\
   or: %s --reset <command>\n", argv[0], argv[0]);
		return 1;
	}

	int arg_index = 1;
	int reset_forced = 0;
	if (strcmp(argv[arg_index], "--reset") == 0)
	{
		arg_index += 1;
		reset_forced = 1;
	}

	char command_input[4096];
	int command_length = 0;
	for (int i = arg_index; i < argc; ++i)
	{
		command_length += strict_snprintf(command_input + command_length, sizeof(command_input) - command_length, "%s ", argv[i]);
	}
	command_input[command_length] = '\0';

	char command_hash[HASH_SIZE + 1];
	hash_command(command_input, command_hash);

	char cache_path[2048];
	{
		const char *home_dir = getenv("HOME");
		strict_snprintf(cache_path, sizeof(cache_path), "%s/.memoize/history", home_dir);
		struct stat st = {0};
		if (stat(cache_path, &st) == -1) {
			mkdir(cache_path, 0700);
		}
	}

	char cache_file[2048];
	strict_snprintf(cache_file, sizeof(cache_file), "%s/%s.txt", cache_path, command_hash);

	if (reset_forced && access(cache_file, F_OK) != -1)
	{
		// Force re-caching when "--reset" is defined
		remove(cache_file);
	}

	if (access(cache_file, F_OK) != -1) {
		// File cache file exists, read and print its content
		FILE *file = fopen(cache_file, "r");
		if (file) {
			char buffer[BUFFER_SIZE];
			while (fgets(buffer, sizeof(buffer), file) != NULL) {
				printf("%s", buffer);
			}
			fclose(file);
		}
	} else {
        // If not, run the command and cache the output
		execute_command(command_input, cache_file);
	}

	return 0;
}
