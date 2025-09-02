if gcc -O3 memoize.c -o memoize.exe -Werror && cp ./memoize.exe ~/mun_bin/ ; then
	echo Built and installed to ~/mun_bin/memoize.exe
	exit 0
fi

echo "Build failed..."
exit 1