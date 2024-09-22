# xobar

colorful i3status alternative
<br>
this program is an lightweight alternative for i3status that can be customized completly with colors and scripts.
<br>
xobar uses a TOML based configuration with very li

the configuration is made up of four primary objects those being the `center`, `left`, `right` and `settings` objects these are required to be defined in your config file. <br><br> 
## installation
to install xobar you can run the following I many terminal, provided `git` and `gcc` are available 
```
git clone https://github.com/9jh1/xobar
cd xobar 
gcc xobar.c -o xobar
sudo cp ./xobar /usr/bin/xobar
xobar --help
```
xobar usage: 
```
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
