// ZS
HMODULE hInpout32;
typedef short (_stdcall *Inp32_Type) (short);
Inp32_Type Inp32_Ptr;
typedef void (_stdcall *Out32_Type) (short, short);
Out32_Type Out32_Ptr;
#define Inp32(a)    (Inp32_Ptr)((a))
#define Out32(a,b)  (Out32_Ptr)((a),(b))


// ZS
int Inpout32_Init(void)
{
    hInpout32 = LoadLibrary("inpout32.dll");
    if (hInpout32 == NULL) return 0;
    
	Inp32_Ptr = (Inp32_Type)GetProcAddress(hInpout32, "Inp32");
    Out32_Ptr = (Out32_Type)GetProcAddress(hInpout32, "Out32");
	if ((Inp32_Ptr == NULL) || (Out32_Ptr == NULL)) return 0;

	return 1;
}
