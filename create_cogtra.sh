cp package/mac80211/Makefile.old.cogtra package/mac80211/Makefile
cp /home/brunoguic/cogtra/920* package/mac80211/patches/
rm package/mac80211/patches/930*

make package/mac80211/clean V=99
make package/mac80211/prepare V=99

cp /home/brunoguic/cogtra/rc80211_cogtra* /home/brunoguic/openwrt/build_dir/linux-ar71xx/compat-wireless-cogtra-2012-03-01/net/mac80211/

make package/mac80211/compile V=99

rm /home/brunoguic/Downloads/cogtra/kmod*

cp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-ath* /home/brunoguic/Downloads/cogtra/
cp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-cfg* /home/brunoguic/Downloads/cogtra/
cp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-mac* /home/brunoguic/Downloads/cogtra/

scp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-ath* rspro:~/cogtra/
scp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-cfg* rspro:~/cogtra/
scp /home/brunoguic/openwrt/bin/ar71xx/packages/kmod-mac* rspro:~/cogtra/
