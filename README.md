# xobar

colorful i3status alternative
<br>
this program is an lightweight alternative for i3status that can be customized completly with colors and scripts.
<br>

## installation
to install xobar you can run the following I many terminal, provided `git` and `gcc` are available 
```BASH
git clone https://github.com/9jh1/xobar
cd xobar 
gcc xobar.c -o xobar
sudo cp ./xobar /usr/bin/xobar
xobar --help
```
xobar usage: 
```
XOBAR (x-oh-bar)
lightweight i3status alternative
********************************
written by 3hy (@9JH1)

xobar [OPTIONS]
 -c [FILE] enter config file path
 -l [INT] update bar every INT seconds
 -v turn on debug prints
 -s [INT] set size of bar
 -r bar resizes to fit terminal
 -t bar prints on newline each update cycle
 --help show this dialog
```
## configuration
xobar uses a TOML based configuration.
the configuration is made up of four primary objects those being the `center`, `left`, `right` and `settings` objects these are required to be defined in your config file. xobar uses a cascading-fallback system for inherited styles for example this configuration
```TOML
# ...
[left] 
children=["example"]
background = "#ff0000"

[example]
type = "text"
content = "Hello World!"
background = "#00ff00" 
# ...
``` 
would show "`Hello World!`" with a green background, removing the third line in `[example]`, `background = "#00ff00"` would result in the "`Hello World!`" text now having a colour of red as the modules background has been overruled by its fallback `[left]` this is because `[example]` is a child of `[left]`. the hierarchy goes as follows; 
```
Module ( example ) 
 Section ( left ) 
  [settings] ( settings ) 
   terminal ( default terminal colours / no colours )
```
### Documentation
- #### Sectors 
  - `spacer` [CHAR] sets the character used for the text of the padding inside of a module
  - `children` [ARRAY] array of module names that will be in this sector
  - `background` [STRING] background colour in hexadecimal format
  - `foreground` [STRING] foreground colour in hexadecimal format
  - `padding-background` [STRING] colour of padding background inside of module in hexadecimal format
  - `padding-foreground` [STRING] colour of padding foreground inside of module in hexadecimal format 
- #### Module
  - `type` [STRING] set type of module, can be set to either `text` or `script`
    - `content` [STRING] if the `type` is set to `text` then the content is the text that will be printed in the module
    - `exec` [STRING] if the `type` is `script` then the exec is the command that will be executed, captured and printed on the module
      - `argument` [STRING] if the `type` is `script` then the argument is parses into the command which will then be executed, captured and printed on the module
  - `background` [STRING] background colour of the module in hexadecimal format
  - `foreground` [STRING] foreground colour of the module in hexadecimal format 
- #### Settings
  - `background` [STRING] background colour of the entire bar ( fallback ) in hexadecimal format
  - `foreground` [STRING] foreground colour of the entire bar ( fallback ) in hexadecimal format 
  - `padding-background` [STRING] background colour of outer padding in hexadecimal format
  - `padding-foreground` [STRING] foreground colour of outer padding in hexadecimal format 
  - `padding-outer` [INT] amount of padding on either side of bar 
  - `padding-inner` [INT] amount of padding between modules inside of bar
  - `spacer` [CHAR] sets the character used for the text of the padding on either side of the bar 
  - `void-spacer` [CHAR] sets the character used for the text of the padding between sectors 
  - `void-background` [STRING] background colour of the padding between sectors 
  - `void-foreground` [STRING] foreground colour of the padding between sectors 