#bin/sh

if [ "$#" -ne 2 ]; then
  echo "expecting two arguments: name and size"
  exit 1
fi

size=$2
twice_size=`expr 2 \* $2`
three_times_size=`expr 3 \* $2`
four_times_size=`expr 4 \* $2`

convert -size "$four_times_size x $three_times_size" xc:white "$1"_result.png
composite -geometry "$size x $size + $size + $size" "$1"_back.ppm "$1"_result.png "$1"_result.png
composite -geometry "$size x $size + $twice_size + $size" "$1"_right.ppm "$1"_result.png "$1"_result.png
composite -geometry "$size x $size + $size + 0" "$1"_top.ppm "$1"_result.png "$1"_result.png
composite -geometry "$size x $size + $size + $twice_size" "$1"_bottom.ppm "$1"_result.png "$1"_result.png
composite -geometry "$size x $size + 0 + $size" "$1"_left.ppm "$1"_result.png "$1"_result.png
composite -geometry "$size x $size + $three_times_size + $size" "$1"_front.ppm "$1"_result.png "$1"_result.png
