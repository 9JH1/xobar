uptime -p | awk -F '[ ,]' '{print $2 " " $3 " " $4}' | sed 's/ //g' | awk '{split($0,a,":"); printf "%02d:%02d:%02d\n", a[1], a[2], 0}' | awk '{printf $0}'
