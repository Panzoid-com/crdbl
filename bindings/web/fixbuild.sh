PREFIX=$(pwd)/../../build/bin
DIST=$(pwd)/dist
TARGET=ProjectDBWeb
OUTPUT=crdbl
SRC=$(pwd)

insert_content=$(<./GetWorker.js)
first_line=$(echo -e "$insert_content" | head -n 1)

#NOTE: this doesn't use the actual first_line content for now because
#  it just won't work
if ! grep -qF "Module[\"getWorker\"]" "$PREFIX/$TARGET.js"; then
  awk -v content="$insert_content" '!found && /return moduleArg.ready/ { print content; found=1 } 1' "$PREFIX/$TARGET.js" > tmp
  mv tmp "$PREFIX/$TARGET.js"
fi

# replace all instances of TARGET string in the file with OUTPUT
sed -i "s/$TARGET/$OUTPUT/g" $PREFIX/$TARGET.js

# call the main file index.js for consistency
cp -f $PREFIX/$TARGET.js $DIST/index.js
cp -f $PREFIX/$TARGET.wasm $DIST/$OUTPUT.wasm
cp -f $SRC/Worker.js $DIST/$OUTPUT.ww.js