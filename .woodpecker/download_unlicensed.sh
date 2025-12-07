#!/bin/bash

mkdir -p icons sounds

curl -L --output icons/icon_category_users.png "https://www.psdevwiki.com/ps3/images/archive/4/47/20140220101344%21Icon_category_users.png"
curl -L --output icons/icon_category_settings.png "https://www.psdevwiki.com/ps3/images/archive/b/ba/20140220101323%21Icon_category_settings.png"
curl -L --output icons/icon_category_photo.png "https://www.psdevwiki.com/ps3/images/archive/6/61/20140220101240%21Icon_category_photo.png"
curl -L --output icons/icon_category_music.png "https://www.psdevwiki.com/ps3/images/archive/2/2b/20140220101209%21Icon_category_music.png"
curl -L --output icons/icon_category_video.png "https://www.psdevwiki.com/ps3/images/archive/c/c3/20140220101141%21Icon_category_video.png"
curl -L --output icons/icon_category_tv.png "https://www.psdevwiki.com/ps3/images/archive/2/27/20140220101110%21Icon_category_tv.png"
curl -L --output icons/icon_category_game.png "https://www.psdevwiki.com/ps3/images/archive/a/af/20140220101052%21Icon_category_game.png"
curl -L --output icons/icon_category_network.png "https://www.psdevwiki.com/ps3/images/archive/e/e6/20140220101001%21Icon_category_network.png"
curl -L --output icons/icon_category_friends.png "https://www.psdevwiki.com/ps3/images/archive/f/f4/20140220100934%21Icon_category_friends.png"

curl -L --output icons/icon_settings_update.png "https://archive.org/download/XMB-Icons/iqvigtkhqlesuj2rins9i3foau-d655f4f0bdf9974ef04884737b7a49c3.png"
mogrify -channel alpha -threshold 99% -trim +repage -background none -gravity Center -extent "300x300<" icons/icon_settings_update.png
