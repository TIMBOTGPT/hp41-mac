#!/bin/bash
# Build HP41.app (arm64). ROM images are not bundled: they are read from
# ~/Library/Application Support/hp41-mac/MOD at run time.
set -e
cd "$(dirname "$0")"
make build/hp41
APP=HP41.app
rm -rf $APP
mkdir -p $APP/Contents/{MacOS,Resources,Frameworks}
cp build/hp41 $APP/Contents/MacOS/hp41
cp -R skin $APP/Contents/Resources/
SDL=$(otool -L build/hp41 | awk '/libSDL2/{print $1}' | head -1)
cp "$SDL" $APP/Contents/Frameworks/libSDL2.dylib
chmod u+w $APP/Contents/Frameworks/libSDL2.dylib
install_name_tool -id @rpath/libSDL2.dylib $APP/Contents/Frameworks/libSDL2.dylib
install_name_tool -change "$SDL" @rpath/libSDL2.dylib $APP/Contents/MacOS/hp41
install_name_tool -add_rpath @executable_path/../Frameworks $APP/Contents/MacOS/hp41
cat > $APP/Contents/Info.plist <<PL
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleName</key><string>HP41</string>
<key>CFBundleDisplayName</key><string>HP-41CX</string>
<key>CFBundleIdentifier</key><string>local.hp41mac.arm64</string>
<key>CFBundleExecutable</key><string>hp41</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleVersion</key><string>0.1</string>
<key>CFBundleShortVersionString</key><string>0.1</string>
<key>LSMinimumSystemVersion</key><string>11.0</string>
<key>NSHighResolutionCapable</key><true/>
</dict></plist>
PL
codesign --force --deep -s - $APP
echo built $APP
