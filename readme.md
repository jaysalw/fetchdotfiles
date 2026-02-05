# Fetch Dot Files

A simple but efficient way to transfer your dotfiles from one setup to another.

## Installation

```sh
make
sudo cp fdf /usr/local/bin/
```

## Usage

```sh
fdf --repo <repo_url> [--force-placement]
fdf -r <repo_url> [-f]
```

**Options:**
- `-r, --repo` - Repository URL (git/GitHub SSH or HTTPS)
- `-f, --force-placement` - Overwrite existing files without prompting

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
- Filenames are case-sensitive on Linux!
- Make sure the destination directory exists before trying to write to it

## License

Do whatever you want with it lol