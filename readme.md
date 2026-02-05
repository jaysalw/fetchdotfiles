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

| Config Key | Description | Default |
|------------|-------------|---------|
| `default_dir` | Subdirectory where dotfiles are stored | `dotfiles` |
| `select_from_root` | If `True`, select files from repo root (ignores `default_dir`) | `False` |

### Commands

```
PUT <filename> IN <destination_path>
EXECUTE "<shell_command>"
ECHO "<message>"
END FETCH
```

| Command | Description |
|---------|-------------|
| `PUT` | Copy a file from repo to destination |
| `EXECUTE` | Run a shell command |
| `ECHO` | Print a message (shows as `[ SCRIPT RESPONSE ]`) |
| `END FETCH` | End of instructions (required) |
| `#` | Comment (line is ignored) |

## Repository Structure

### Default (using `dotfiles/` subdirectory):

```
your-repo/
├── mydotfiles.fdf
└── dotfiles/
    ├── .bashrc
    ├── .vimrc
    └── config.conf
```

**mydotfiles.fdf:**
```
PUT .bashrc IN /home/user/.bashrc
PUT .vimrc IN /home/user/.vimrc
ECHO "Dotfiles installed!"
END FETCH
```

### Custom directory:

```
your-repo/
├── setup.fdf
└── configs/
    └── nginx.conf
```

**setup.fdf:**
```
CONFIG default_dir = configs
PUT nginx.conf IN /etc/nginx/nginx.conf
END FETCH
```

### From root (no subdirectory):

```
your-repo/
├── install.fdf
├── .bashrc
└── .zshrc
```

**install.fdf:**
```
CONFIG select_from_root = True
PUT .bashrc IN /home/user/.bashrc
PUT .zshrc IN /home/user/.zshrc
END FETCH
```

## Example Output

```
[ INFO ] Cloning repository: https://github.com/user/dotfiles
[ INFO ] Found: repo_tmp/setup.fdf
[ TASK ] Processing dotfile...

[ CONFIG ] default_dir = configs
[ TASK ] Placing nginx.conf -> /etc/nginx/nginx.conf
[ SUCCESS ] Placed nginx.conf -> /etc/nginx/nginx.conf
[ SCRIPT RESPONSE ] Installation complete!
[ INFO ] End of fetch instructions

[ SUCCESS ] Dotfiles applied successfully!
```

## Security

For private repositories, use SSH keys:
```sh
fdf -r git@github.com:username/private-repo.git
```