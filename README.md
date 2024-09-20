# xobar

colorful i3status alternative
<br>
this program is an lightweight alternative for i3status that can be customized completly with colors and scripts.
<br>
xobar uses a TOML based configuration like so

```TOML
# default configuration
[left]
spacer = " "
children = ["left_example"]

[center]
spacer = " "
children = ["center_example"]
background = "#FF0000"
prefix-tail = "<"
suffix-tail = ">"
tail-foreground = "#FF0000"


[right]
spacer = " "
children = ["right_example"]

[left_example]
type="script"
exec = "./exec.sh"

[center_example]
type="text"
content="center "

[right_example]
type="text"
content="right"

[settings]
background = "#00ff00"
foreground = "#ff0000"
spacer = " "
padding = 0
padding-inner = 2
```

the configuration is made up of four primary objects those being the `center`, `left`, `right` and `settings` objects these are required

### settings

the settings object holds the main styles for the bar, it holds the padding sizes and the default colors

### left, right & center

these are the default styles for the left right and center sectors they can be styled just like the settings box. if not styled the sectors colors will fall back to whats in the settings and if there are no settings colors then it will fall back to default terminal colors
