# Fetch Dot Files

A simple but efficient way to transfer your dotfiles from one setup to another.

**Website:** [fetchdots.net](https://fetchdots.net)

Want an example repository? See https://github.com/jaysalw/fdf_testing/


**NOTES:**
I am currently creating a **website** for this project using the domain **fetchdots.net** and will include:

* Script Validator
* In-depth documentation
* Release changelog with binaries and pre-compiled versions

And more :)

## Installation

If you're on Arch Linux and have an AUR helper such as ``yay`` installed, you can quickly install the latest STABLE release of **fetchdotfiles** by running:

```
yay -Sy fetchdots
```

I plan to add this to other package managers when I have the time.

**Dependencies:**
- `ncurses` - Required for the diff viewer TUI

```sh
# Debian/Ubuntu
sudo apt install libncurses-dev

# Fedora
sudo dnf install ncurses-devel

# Arch
sudo pacman -S ncurses
```

**Build & Install:**
```sh
make
sudo cp fdf /usr/local/bin/
```

## Usage

```sh
fdf --repo <repo_url> [--force-placement] [--show-diff]
fdf -r <repo_url> [-f] [-d]
fdf rollback
fdf docs
```

**Options:**
- `-r, --repo` - Repository URL (git/GitHub SSH or HTTPS)
- `-f, --force-placement` - Overwrite existing files without prompting
- `-d, --show-diff` - Show a side-by-side diff in a TUI before placing each file
- `rollback` - Undo the latest file changes made by fdf
- `docs` - View fdf documentation in ncurses

## FDF Language Syntax

### Configuration Commands

Place these at the top of your `.fdf` file:

```
CONFIG default_dir = myfiles
CONFIG select_from_root = True
```

- `default_dir` - Where your dotfiles are stored in the repo (default: `dotfiles`)
- `select_from_root` - Set to `True` if you want to grab files straight from the repo root instead of a subfolder

### Commands

```
PUT <filename> IN <destination_path>
EXECUTE "<shell_command>"
ECHO "<message>"
END FETCH
```

- `PUT` - Copies a file from your repo to wherever you want it
- `EXECUTE` - Runs a shell command
- `ECHO` - Prints a message to the terminal
- `END FETCH` - Tells fdf you're done (always put this at the end!)
- `#` - Comments, fdf will ignore these lines

## How to Set Up Your Repo

By default fdf looks for files in a `dotfiles/` folder. So your repo should look something like:

```
my-dotfiles-repo/
├── setup.fdf
└── dotfiles/
    ├── .bashrc
    ├── .vimrc
    └── whatever.conf
```

And your `setup.fdf` would be:

```
# My dotfiles setup script
PUT .bashrc IN ~/.bashrc
PUT .vimrc IN ~/.vimrc
PUT whatever.conf IN /etc/whatever.conf
ECHO "All done!"
END FETCH
```

### Using a Different Folder

Don't want to use `dotfiles/`? No problem:

```
CONFIG default_dir = configs

PUT nginx.conf IN /etc/nginx/nginx.conf
END FETCH
```

### Grabbing Files From Root

If you just want everything in the root of your repo:

```
CONFIG select_from_root = True

PUT .bashrc IN ~/.bashrc
PUT .zshrc IN ~/.zshrc
END FETCH
```

## Example

```sh
$ ./fdf -r https://github.com/myuser/my-dotfiles

[ INFO ] Cloning repository: https://github.com/myuser/my-dotfiles
[ INFO ] Found: repo_tmp/setup.fdf
[ TASK ] Processing dotfile...

[ TASK ] Placing .bashrc -> /home/user/.bashrc
[ SUCCESS ] Placed .bashrc -> /home/user/.bashrc
[ SCRIPT RESPONSE ] All done!
[ INFO ] End of fetch instructions
[ SUCCESS ] Dotfiles applied successfully!
```

## Tips

- Use SSH for private repos: `fdf -r git@github.com:user/private-repo.git`
- Use `-f` flag to skip the "overwrite?" prompts
- Use `-d` flag to see a side-by-side diff before files are placed
- If the last fetch changed files you don't want, run `fdf rollback`
- Filenames are case-sensitive on Linux!
- Make sure the destination directory exists before trying to write to it

## Rollback

Running `fdf rollback` reverts file placements from the most recent fetch run.

- Files that were overwritten are restored from backup.
- Files that were newly created by fdf are removed.
- Rollback currently targets `PUT` file operations only.

## Documentation

Running `fdf docs` opens an ncurses-based documentation viewer with comprehensive usage information, syntax examples, and troubleshooting tips.

You can customize the documentation by editing `~/.fdf_docs.txt` with your favorite text editor. It will automatically display when you run `fdf docs`.

**Navigation in docs viewer:**
- `q` or `ESC` - Exit viewer
- `↑`/`↓` - Scroll line by line  
- `PgUp`/`PgDn` - Scroll by page
- `Home`/`End` - Jump to start/end

## Diff Viewer

When using `--show-diff` or `-d`, fdf will open an ncurses-based TUI showing a side-by-side comparison of the existing file and the new file before each placement.

**Controls:**
- `↑`/`↓` or `j`/`k` - Scroll line by line
- `PgUp`/`PgDn` - Scroll by page
- `Home`/`End` or `g`/`G` - Jump to start/end
- `q` or `ESC` - Close the diff viewer and continue

**Color Legend:**
- 🟢 Green (`+`) - Lines added in the new file
- 🔴 Red (`-`) - Lines removed from the existing file  
- 🟡 Yellow (`~`) - Lines that were changed

## License

Do whatever you want with it lol (planning on using an open source license!!)
