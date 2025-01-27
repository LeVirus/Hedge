#!/bin/bash


calc(){ awk "BEGIN { print "$*" }"; }

#1 first X pos, 
#2 Y up position, 
#3 SECOND X, 
#4 Y down position, 
#5 offset between sprite
#6 number of pattern on the line
#7 texture name
#8 texture engine number
#9 name group sprite

if [ $# -ne 9 ]
then
	echo "Error bad number of arguments. $# "
	exit
fi
if [ ! -f $7 ]
then
	echo "Texture $7 not found"
fi

XTextureSize=$(file $7 | cut -d ' ' -f 5)
YTextureSize=$(file $7 | cut -d ' ' -f 7 | cut -d ',' -f 1)

currentX=$(calc $1/$XTextureSize)
yUpPos=$(calc $2/$YTextureSize)

sizeSpriteX=$(calc $(calc $3-$1)/$XTextureSize)
sizeSpriteY=$(calc $(calc $4-$2)/$YTextureSize)

cmptNum=0
cmpt='A'
offsetX=$(calc $5/$XTextureSize)
while [ $cmptNum -ne $6 ] 
do
	echo
	echo "[Sprite$9$cmpt]"
	echo "texture = $8"
	echo "texturePosX = $currentX"
	echo "texturePosY = $yUpPos"
	echo "textureWeight = $sizeSpriteX"
	echo "textureHeight = $sizeSpriteY"
	cmptNum=$(($cmptNum+1))
	cmpt="$(echo $cmpt | tr '[A-Y]Z' '[B-Z]A')"
	currentX=$(calc $currentX+$offsetX)
done


