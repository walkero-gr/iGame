---
	tags:
	- datatypes
	- images
	- bitmap
---
This is a WIP. Doesn't fully work, and it needs a better way to make it load in the window.

```c
Object *dtObj = NewDTObject(DEFAULT_SCREENSHOT_FILE,
		DTA_GroupID,	GID_PICTURE,
		TAG_DONE);

if (dtObj)
{
	printf("DBG: datatype object created fine\n");

	struct BitMapHeader *bmhd = NULL;

	ULONG res = GetDTAttrs(dtObj,
			PDTA_BitMapHeader, &bmhd,
			TAG_DONE);

	if(bmhd)
	{
		printf("DBG: BitMapHeader created fine\n");
		printf("BMHD width: %ld, height: %ld \n", bmhd->bmh_Width, bmhd->bmh_Height);

		struct BitMap *bitmapSrc = AllocBitMap(bmhd->bmh_Width, bmhd->bmh_Height, 8, BMF_CLEAR, NULL);

		if(bitmapSrc)
		{
			printf("DBG: AllocBitMap created fine\n");
			InitBitMap(&bitmapSrc, smiley8_Depth, smiley8_Width, smiley8_Height);
			object->IM_GameImage_0 = MUI_NewObject(MUIC_Bitmap,
					MUIA_Bitmap_Bitmap,       &bitmapSrc,
					//MUIA_Bitmap_Precision,    0,
					// MUIA_Bitmap_SourceColors, (ULONG *)image_palette,
					MUIA_Bitmap_Transparent,  0,
					MUIA_Bitmap_Height,       bmhd->bmh_Height,
					MUIA_Bitmap_Width,        bmhd->bmh_Width,
					MUIA_FixHeight,           bmhd->bmh_Height,
					MUIA_FixWidth,            bmhd->bmh_Width,
				End;

			//FreeBitMap(bitmapSrc);
		}
		else
		{
			printf("DBG: AllocBitMap failed\n");
		}

	}
	else
	{
		printf("DBG: BitMapHeader failed\n");
	}




	DisposeDTObject (dtObj);
}
else
{
	printf("DBG: datatype object failed\n");
}
```
