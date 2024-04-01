JAVA_HOME=/usr/lib/jvm/java-8-oracle/bin

rm -r bin/love_decoded
rm -r bin/*.apk
rm -r bin/*.love
cd game/
zip -9 -r game.love .
cp -r game.love ../bin/game.love
cp -r game.love ../android/app/src/main/assets/game.love
rm -r game.love
cd ../android/
# ./gradlew
# ./gradlew tasks

PS3='Please enter your choice: '
options=("gradlew build" "gradlew assembleDebug" "gradlew installDebug" "Quit")
select opt in "${options[@]}"
do
    case $opt in
        "gradlew build")
            ./gradlew build  --max-workers=8
            cp -r app/build/outputs/apk/debug/app-debug.apk ../bin/app-debug.apk
            cp -r app/build/outputs/apk/release/app-release-unsigned.apk ../bin/app-release-unsigned.apk
            #blind2403%%
            cd ../bin/
            jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore perseguidor-key.keystore app-release-unsigned.apk perseguidor -storepass blind2403%%
            /root/Android/Sdk/build-tools/28.0.3/zipalign -v 4 /home/lulz/Projects/PERSEGUIDOR/bin/app-release-unsigned.apk /home/lulz/Projects/PERSEGUIDOR/bin/Perseguidor.apk
            
            apktool d -s -o love_decoded Perseguidor.apk
            
            break
            ;;
        "gradlew assembleDebug")
            ./gradlew assembleDebug --max-workers=8
            cp -r app/build/outputs/apk/debug/app-debug.apk ../bin/app-debug.apk
            cp -r app/build/outputs/apk/release/app-release-unsigned.apk ../bin/app-release-unsigned.apk
            break
            ;;
        "gradlew installDebug")
            ./gradlew installDebug --max-workers=8
            break
            ;;
        "Quit")
            break
            ;;
        *) echo "invalid option $REPLY";;
    esac
done
