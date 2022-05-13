INDIR=$1
OUTDIR=$2
FILEPATTERN=$3

for f in `cd $INDIR && find -name $FILEPATTERN`
do dirn=$(dirname "$f")
   mkdir -p "$OUTDIR/$dirn"
   mv "$INDIR/$f" "$OUTDIR/$f"
done
