Cores=2
if ! [ -z "$1" ]
then
  Cores="$1"
fi

chmod +x build_Umsci_RaspberryPIOS.sh
./build_Umsci_RaspberryPIOS.sh $Cores

# we are in Resources/Deployment/Linux/ -> change directory to project root
cd ../../../

# set convenience variables
UmsciBinaryPath=Builds/LinuxMakefile/build/Umsci
ChangeLogPath=CHANGELOG.md
LicensePath=LICENSE
DependencyInstallScript=get_dependencies_RaspberryPIOS.sh
DependencyInstallScriptPath="Resources/Deployment/Linux/""$DependencyInstallScript"
UmsciZipTargetPath=Umsci.zip´
PackageContentCollectionPath=PackageContentCollection

mkdir "$PackageContentCollectionPath"

# copy required assets into temp collection folder
cp "$DependencyInstallScriptPath" "$PackageContentCollectionPath/$DependencyInstallScript"
cp "$ChangeLogPath" "$PackageContentCollectionPath/$ChangeLogPath"
cp "$LicensePath" "$PackageContentCollectionPath/$LicensePath"
mv "$UmsciBinaryPath" "$PackageContentCollectionPath/Umsci"

# change directory into collection folder
cd "$PackageContentCollectionPath"

# Umsci app release package
test -f "$UmsciZipTargetPath" && rm "$UmsciZipTargetPath"
zip "$UmsciZipTargetPath" "Umsci" "$ChangeLogPath" "$LicensePath" "$DependencyInstallScript"

# back to root for further package handling
cd ..
cp "$PackageContentCollectionPath/$UmsciZipTargetPath" "$UmsciZipTargetPath"

# cleanup collection folder
test -d "$PackageContentCollectionPath" && rm -R "$PackageContentCollectionPath"
