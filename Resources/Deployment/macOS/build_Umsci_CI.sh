# we are in Resources/Deployment/macOS/ -> change directory to project root
cd ../../../

# set convenience variables
JUCEDir=submodules/JUCE
ProjucerPath="$JUCEDir"/extras/Projucer/Builds/MacOSX
ProjucerBinPath="$ProjucerPath"/build/Release/Projucer.app/Contents/MacOS/Projucer
JucerProjectPath=Umsci.jucer
XCodeProjectPath=Builds/MacOSX/Umsci.xcodeproj

# build projucer
xcodebuild -project "$ProjucerPath"/Projucer.xcodeproj -configuration Release -jobs 8

# export projucer project
"$ProjucerBinPath" --resave "$JucerProjectPath" --fix-missing-dependencies

# start building the project. The provisioning profile specification refers to a profile manually created and physically present on the build machine...
# CODE_SIGN_IDENTITY="-" overrides the real "Developer ID Application" identity baked into
# Umsci.jucer's Release config with ad-hoc signing -- CODE_SIGNING_ALLOWED=NO alone does not
# suffice because Projucer's "Sign Target" script phase runs unconditionally as a raw shell
# script (not Xcode's native code-signing step) and would otherwise fail with "no identity
# found" on a CI runner that has no such certificate installed.
xcodebuild -project "$XCodeProjectPath" -configuration Release -jobs 8 CODE_SIGNING_ALLOWED=NO CODE_SIGN_IDENTITY="-"
