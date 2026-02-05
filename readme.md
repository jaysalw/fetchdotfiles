# Fetch Dot Files
A simple but efficient way to transfer your dotfiles from one setup to another. 

Using a predefined dotfile system.
```
PUT <dotfile> IN <file_path>
EXECUTE "<command>"
```

This is then placed into the stated file path, and any listed commands are executed. Essentially you can have the ran in one single script - e.g. located in a private GIT repository or a private github repo.


```fdf --repo username@password:repository --force-placement``

### Repository File Structure
Your .fdf files should be in the **ROOT** of your repository, then place your dotfiles in any folder you like. You are able to select the path to said file from your scripts.