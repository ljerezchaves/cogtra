echo "Turning WiFi off..."
wifi down

echo "Removing packages..."
rmmod ath9k
rmmod ath5k
rmmod ath9k-htc
rmmod ath9k-common
rmmod ath
rmmod mac80211
rmmod cfg80211

echo "Uninstalling existing packages..."
opkg remove kmod-ath9k
opkg remove kmod-ath5k
opkg remove kmod-ath9k-htc
opkg remove kmod-ath9k-common
opkg remove kmod-ath
opkg remove kmod-mac80211
opkg remove kmod-cfg80211

echo "Installing new packages..."
opkg install kmod-cfg80211_*.ipk
opkg install kmod-mac80211_*.ipk 
opkg install kmod-ath_*.ipk
opkg install kmod-ath9k-common_*.ipk
opkg install kmod-ath9k-htc_*.ipk
opkg install kmod-ath5k_*.ipk
opkg install kmod-ath9k_*.ipk 

echo "Turning WiFi on..."
wifi up

echo "DONE!"
exit 0
