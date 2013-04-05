cp package/mac80211/Makefile.old package/mac80211/Makefile
rm package/mac80211/patches/920*
rm package/mac80211/patches/930*

make package/mac80211/clean V=99
make package/mac80211/prepare V=99

cp /home/brunoguic/cogtra/rc80211_minstrel* /home/brunoguic/openwrt/build_dir/linux-ar71xx/compat-wireless-2011*/net/mac80211/

make package/mac80211/compile V=99

rm /home/brunoguic/Downloads/minstrel/kmod*

cp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-ath* /home/brunoguic/Downloads/minstrel/
cp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-cfg* /home/brunoguic/Downloads/minstrel/
cp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-mac* /home/brunoguic/Downloads/minstrel/


scp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-ath* rspro:~/minstrel/
scp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-cfg* rspro:~/minstrel/
scp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-mac* rspro:~/minstrel/
