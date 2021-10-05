#include <io.h>

/* Generate enough dummy data to make the output .exe go over 40 kB, which
   seems to be the trigger point at which PKLite enables "large" mode. */

#define D "123456789012345678901234567890123456789012345678901234567890"
#define D2  D D D D D D D D D D  D D D D D D D D D D  D D D D D D D D D D
#define D3  D2 D2 D2 D2 D2 D2 D2 D2 D2 D2  D2 D2 D2 D2 D2 D2 D2 D2 D2 D2 "abcd"

int main(void)
{
	write(0, D3, sizeof(D3));
	return 0;
}
