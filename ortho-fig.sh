#!/bin/sh
set -e    # exit on error (except commands bracketed with "set +e ... set -e")
#
# Script to run the CHAP orthology figure generator.
#

# these location values may be changed by "make install"
SCRIPTS=..
JARS=..
BIN=../bin
RESOURCES=../resources

JAVA=java
JAR=$JARS/orthofig.jar
MAIN_SCRIPT=$SCRIPTS/ortho.sh             # for error message
USER_SEQ=seq.d                            # original user-provided sequences
X_ORTHO=ortho.d/x-ortho.d                 # X-orthology results
N_ORTHO=ortho.d/n-ortho.d                 # N-orthology results
FIG_ANNOT=fig_annot.d                     # gene annotations, including inferred pseudo-genes
FIGURES=figures.d                         # files for automatic PostScript figures
COLOR_LIST=$RESOURCES/ortho-fig.colors    # overrides default gene colors for figure (more colors)
TEMP=temp.d                               # assorted temporary files
CONTIGS=contigs.d                               # assorted temporary files
SCRIPT=`echo $0 | sed -e 's;.*/;;'`       # script name from command line; path removed for msgs

if [ $# -lt 1 ] || [ $# -gt 2 ]
then
	echo "Usage:  $SCRIPT refseq_name [annot_dir]"
	exit 1
fi

ref_sp=$1
if [ $# -gt 1 ]
then
	ANNOT_DIR=$2
else
	ANNOT_DIR=$FIG_ANNOT
fi

if [ ! -f $USER_SEQ/$ref_sp ]
then
	echo "Sequence matching reference name \"$ref_sp\" is not found in $USER_SEQ."
	exit 1
fi

if [ ! -d $X_ORTHO ] || [ ! -d $N_ORTHO ] || [ ! -f non-redundant.gc ]
then
	echo "Directories \"$X_ORTHO\", \"$N_ORTHO\", and/or"
	echo "conversion output file \"non-redundant.gc\" are not found."
	echo "Please run the main script \"$MAIN_SCRIPT\" first to create them."
	exit 1
fi

if [ ! -d $ANNOT_DIR ]
then
	echo "Directory \"$ANNOT_DIR\" is not found."
	if [ $ANNOT_DIR = $FIG_ANNOT ]
	then
		echo "Please run the main script \"$MAIN_SCRIPT\" first to create it."
	fi
	exit 1
fi

set +e
which $JAVA > /dev/null 2>&1
if [ $? -ne 0 ]
then
	echo "Program \"$JAVA\" is not found."
	echo "Please install Java and make it available from your command path."
	exit 1
fi
set -e


# rm -rf $TEMP; mkdir -p $TEMP

echo "creating input files for the orthology figure generator"

$BIN/print_genes $ANNOT_DIR/$ref_sp.codex $ref_sp $CONTIGS/all.contigs.list > $TEMP/x-ortho.fig
for sp in `ls $USER_SEQ`
do
	if [ $sp != $ref_sp ] && [ -f $ANNOT_DIR/$sp.codex ]
	then
		$BIN/ortho_view_info $X_ORTHO/$ref_sp.$sp.maf $ANNOT_DIR/$ref_sp.codex $ANNOT_DIR/$sp.codex $CONTIGS/all.contigs.list >> $TEMP/x-ortho.fig
	fi
done

$BIN/print_genes $ANNOT_DIR/$ref_sp.codex $ref_sp $CONTIGS/all.contigs.list > $TEMP/n-ortho.fig
for sp in `ls $USER_SEQ`
do
	if [ $sp != $ref_sp ] && [ -f $ANNOT_DIR/$sp.codex ]
	then
		$BIN/ortho_view_info $N_ORTHO/$ref_sp.$sp.maf $ANNOT_DIR/$ref_sp.codex $ANNOT_DIR/$sp.codex $CONTIGS/all.contigs.list >> $TEMP/n-ortho.fig
	fi
done

mkdir -p $FIGURES
rm -f $FIGURES/$ref_sp.x-ortho.fig
rm -f $FIGURES/$ref_sp.n-ortho.fig
$BIN/renumber $TEMP/x-ortho.fig non-redundant.gc $TEMP/x-ortho.param.txt $ref_sp > $FIGURES/$ref_sp.x-ortho.fig
$BIN/renumber $TEMP/n-ortho.fig non-redundant.gc $TEMP/n-ortho.param.txt $ref_sp > $FIGURES/$ref_sp.n-ortho.fig
	# x-ortho.param.txt, n-ortho.param.txt are secondary output files

echo "running the orthology figure generator"

num_sp=1
for sp in `ls $USER_SEQ`
do
	if [ $sp != $ref_sp ]
	then
		num_sp=`expr $num_sp + 1`
	fi
done
height=`expr $num_sp \* 25`    # points
height=`expr $height / 72`     # inches (with default dpi)
height=`expr $height + 1`      # margins
if [ $height -lt 4 ]
then
	height=4
fi

#x_chars=`$BIN/count_char $FIGURES/$ref_sp.x-ortho.fig`
#n_chars=`$BIN/count_char $FIGURES/$ref_sp.n-ortho.fig`

rm -f $FIGURES/$ref_sp.x-ortho.eps
rm -f $FIGURES/$ref_sp.n-ortho.eps

param=`cat $TEMP/x-ortho.param.txt`
gwidth=`echo $param | cut -d ' ' -f1`
sfs=`echo $param | cut -d ' ' -f2`
gfs=`echo $param | cut -d ' ' -f3`
gla=`echo $param | cut -d ' ' -f4`
if [ ! -z `echo ${gwidth}${sfs}${gfs}${gla} | sed -e 's;[0-9.];;g'` ]
then
	echo "Invalid characters in intermediate file $TEMP/x-ortho.param.txt."
	exit 1
fi
cmd="$JAVA -jar $JAR -size 6 $height -gsize $gwidth 20 -sfs $sfs -gfs $gfs -gla $gla -colors $COLOR_LIST $FIGURES/$ref_sp.x-ortho.fig > $FIGURES/$ref_sp.x-ortho.eps"
echo "  $cmd"
eval $cmd    # Note: eval is unsafe if variable contents are untrusted;
             # also beware of special characters that need escaping.
             # Here the values are simple constant filenames, along
             # with numbers generated by the program.  Theoretically
             # another user could tamper with the *.param.txt file
             # though, hence the check for invalid characters.

param=`cat $TEMP/n-ortho.param.txt`
gwidth=`echo $param | cut -d ' ' -f1`
sfs=`echo $param | cut -d ' ' -f2`
gfs=`echo $param | cut -d ' ' -f3`
gla=`echo $param | cut -d ' ' -f4`
if [ ! -z `echo ${gwidth}${sfs}${gfs}${gla} | sed -e 's;[0-9.];;g'` ]
then
	echo "Invalid characters in intermediate file $TEMP/n-ortho.param.txt."
	exit 1
fi
cmd="$JAVA -jar $JAR -size 6 $height -gsize $gwidth 20 -sfs $sfs -gfs $gfs -gla $gla -colors $COLOR_LIST $FIGURES/$ref_sp.n-ortho.fig > $FIGURES/$ref_sp.n-ortho.eps"
echo "  $cmd"
eval $cmd    # see Note above

# rm -rf $TEMP