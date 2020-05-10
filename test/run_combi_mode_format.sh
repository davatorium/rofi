rofi -show combi -combi-modi "window,drun" -eh 2 \
	-drun-display-format "[<span weight='light' size='small'><i>({generic})</i></span>]"$'\n'"{name}" \
	-combi-display-format "[<span weight='light' size='small'><i>[{mode}]</i></span>]{linebreak}{element}" \
	-combi-no-linebreak-str " " \
	-markup-combi 1 \
	-combi-no-linebreak-modi "drun"
