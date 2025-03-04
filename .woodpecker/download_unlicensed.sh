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

curl -L --output icons/icon_button_xbox_ok.png "https://upload.wikimedia.org/wikipedia/commons/thumb/d/d2/Xbox_button_A.svg/240px-Xbox_button_A.svg.png"
curl -L --output icons/icon_button_xbox_cancel.png "https://upload.wikimedia.org/wikipedia/commons/thumb/b/b8/Xbox_button_B.svg/240px-Xbox_button_B.svg.png"
curl -L --output icons/icon_button_xbox_option.png "https://upload.wikimedia.org/wikipedia/commons/thumb/d/df/Xbox_button_Y.svg/240px-Xbox_button_Y.svg.png"
curl -L --output icons/icon_button_xbox_extra.png "https://upload.wikimedia.org/wikipedia/commons/thumb/8/8c/Xbox_button_X.svg/240px-Xbox_button_X.svg.png"
curl -L --output icons/icon_button_xbox_up.png "https://upload.wikimedia.org/wikipedia/commons/thumb/6/60/Xbox_D-pad_%28U%29.svg/240px-Xbox_D-pad_%28U%29.svg.png"
curl -L --output icons/icon_button_xbox_down.png "https://upload.wikimedia.org/wikipedia/commons/thumb/e/e3/Xbox_D-pad_%28D%29.svg/240px-Xbox_D-pad_%28D%29.svg.png"
curl -L --output icons/icon_button_xbox_left.png "https://upload.wikimedia.org/wikipedia/commons/thumb/5/57/Xbox_D-pad_%28L%29.svg/240px-Xbox_D-pad_%28L%29.svg.png"
curl -L --output icons/icon_button_xbox_right.png "https://upload.wikimedia.org/wikipedia/commons/thumb/6/65/Xbox_D-pad_%28R%29.svg/240px-Xbox_D-pad_%28R%29.svg.png"
curl -L --output icons/icon_button_xbox_home.png "https://upload.wikimedia.org/wikipedia/commons/thumb/e/e5/Xbox_Logo.svg/239px-Xbox_Logo.svg.png"
mogrify +level-colors "#c4c3c4" icons/icon_button_xbox_{up,down,left,right,home}.png

curl -L --output icons/icon_button_playstation_ok.png "https://upload.wikimedia.org/wikipedia/commons/thumb/9/91/PlayStationCross.svg/288px-PlayStationCross.svg.png"
curl -L --output icons/icon_button_playstation_cancel.png "https://upload.wikimedia.org/wikipedia/commons/thumb/5/58/PlayStationCircle.svg/288px-PlayStationCircle.svg.png"
curl -L --output icons/icon_button_playstation_options.png "https://upload.wikimedia.org/wikipedia/commons/thumb/7/7b/PlayStationTriangle.svg/288px-PlayStationTriangle.svg.png"
curl -L --output icons/icon_button_playstation_extra.png "https://upload.wikimedia.org/wikipedia/commons/thumb/4/42/PlayStationSquare.svg/288px-PlayStationSquare.svg.png"
mogrify -background none -gravity Center -extent "300x300<" icons/icon_button_playstation_{ok,cancel,options,extra}.png
