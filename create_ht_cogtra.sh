cp package/mac80211/Makefile.old.cogtra_ht package/mac80211/Makefile
cp /home/brunoguic/cogtra/920* package/mac80211/patches/
cp /home/brunoguic/cogtra/930* package/mac80211/patches/

make package/mac80211/clean V=99
make package/mac80211/prepare V=99

cp /home/brunoguic/cogtra/rc80211_cogtra_ht* /home/brunoguic/openwrt/build_dir/linux-ar71xx/compat-wireless-cogtra-2012-03-01/net/mac80211/

make package/mac80211/compile V=99

rm /home/brunoguic/Downloads/cogtra_ht/kmod*

cp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-ath* /home/brunoguic/Downloads/cogtra_ht/
cp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-cfg* /home/brunoguic/Downloads/cogtra_ht/
cp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-mac* /home/brunoguic/Downloads/cogtra_ht/

scp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-ath* rspro:~/cogtra_ht/
scp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-cfg* rspro:~/cogtra_ht/
scp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-mac* rspro:~/cogtra_ht/
