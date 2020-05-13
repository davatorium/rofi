rofi -show combi \
	-modi "test:./test_script.sh" \
	-combi-modi "test,window,drun" -eh 2 \
	-drun-display-format "[<span weight='light' size='small'><i>({generic})</i></span>]"$'\n'"{name}" \
	-combi-display-format "[<span weight='light' size='small'><i>[{mode}]</i></span>]{linebreak}{element}" \
	-combi-no-linebreak-str " " \
	-combi-no-linebreak-modi "drun" \
	-markup-combi 1
