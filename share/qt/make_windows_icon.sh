#!/bin/bash
# create multiresolution windows icon
ICON_DST=../../src/qt/res/icons/TorrentCoin.ico

convert ../../src/qt/res/icons/TorrentCoin-16.png ../../src/qt/res/icons/TorrentCoin-32.png ../../src/qt/res/icons/TorrentCoin-48.png ${ICON_DST}
