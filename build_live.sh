J86_APP=$(pwd)
cd ${J86_APP}/jni/

pwd

echo "......"
echo "---------- build for ARM -------------"
echo "......"

/home/sumit/Downloads/android-ndk-r8/ndk-build V=1 -C ${J86_APP}/jni/
