Plug this between `fzf` and `fd-find` to get to filtering faster. Add a ctrl-r bind to for reloading.

# Building
```sh
gcc memoize.c -o memoize
```

# Usage
```sh
memoize <command>
```
The above runs `<command>` and saves its `stdout` into `~/.memoize/history/<hash>.txt`
where the `<hash>` is generated from the given `<command>` and the current working directory.

## `fzf` and `fdfind`
Example usage with `fzf` with `fdfind`:
```sh
memoize fdfind --hidden | fzf
```
For this example to work install `fzf` and `fd-find` packages. I recommend getting the `fzf` binary from github as the one in distro packages might be super old.

To hook this into the `fzf` default behavior, add this to your `.bashrc` (or `.config/fish/config.fish` if you use `fish`)
```sh
export FZF_DEFAULT_COMMAND='memoize fdfind --hidden'
export FZF_DEFAULT_OPTS="--bind 'ctrl-r:reload(memoize --reset fdfind --hidden)'"
```
and run `fzf` normally to get the caching behavior. When the results get too stale, press `cltr-r` to re-generate the cache for the current command and working directory.

## My setup
In `~/.config/fish/config.fish`
```shif
export C_SCRATCH_FILE=$HOME/.local/c_scratch_file.sh
function c
    # Swap between two locations
    set old_pwd $(pwd)
    if not test -e $C_SCRATCH_FILE
        echo "cd '$old_pwd'" > $C_SCRATCH_FILE
    end

    source $C_SCRATCH_FILE
    echo "cd '$old_pwd'" > $C_SCRATCH_FILE
end

export FZF_DEFAULT_COMMAND='memoize.exe fdfind --hidden'
export FZF_DEFAULT_OPTS_FILE=$HOME/.local/fzf-default-flags.txt
function fzf-result-handler
    # Save path to clipboard (both the terminal and the normal one)
    set path "$argv[1]"
    set clip_path "$temp_clipboard_prefix$path"
    echo "$clip_path" | tr -d "\n" | xsel --primary && xsel --primary --output | xsel --clipboard

    # Save path to a scratch file so that `c` can swap to it
    set absolute_path (realpath $path)
    if test -f $absolute_path
        set absolute_path (dirname $absolute_path)
    end
    echo "cd '$absolute_path'" > $C_SCRATCH_FILE
end

function fz
    # Run fzf at home
    set old_pwd $(pwd)
    cd ~
    set --universal temp_clipboard_prefix "~/"
    set result (fzf --bind 'ctrl-h:execute-silent()')
    if test -n "$result"
        if test -e "$HOME/$result"
            echo "~/$result"
        else
            echo "$result"
        end
    end
    set --erase temp_clipboard_prefix
    cd $old_pwd
end
```

In `~/.local/fzf-default-flags.txt`
```sh
--height=~60%
--layout=reverse
--info=inline
--tiebreak=pathname,length
--exact
--scheme=path
--bind 'enter:execute-silent(fzf-result-handler {})+accept'
--bind 'ctrl-space:execute-silent(xdg-open {})'
--bind 'ctrl-r:reload(memoize.exe --reset fdfind --hidden)'
--bind 'ctrl-o:reload-sync(memoize.exe --reset fdfind --hidden)'
--bind 'ctrl-h:become(fz)'
```
