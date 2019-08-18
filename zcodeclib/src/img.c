#include "general.h"
#include "mfdb.h"
#include "load_img.h"
#include "img.h"

IMAGE *init_img( int16 page, int16 w, int16 h, int16 planes)
{
	IMAGE *data;
	int16 i;

	data = ( IMAGE *)malloc( sizeof( IMAGE));
	
	if( data == NULL)
		return NULL;
	
	data->page	= page;
	data->delay	= NULL;

	data->image = ( MFDB *)malloc( sizeof( MFDB) * page);

	if( data->image == NULL)
	{
		free( data);
		return NULL;
	}

	for ( i = 0 ; i < page ; i++)	
	{
		if ( !init_mfdb( &data->image[i], w, h, planes))
		{
			if( i)
				delete_mfdb( data->image, i + 1);

			free( data->image);
			free( data);	
			return NULL;
		}
	}

	return data;
}

void delete_img( IMAGE *data)
{
	if( data == NULL)
		return;

	delete_mfdb( data->image, data->page);

	if( data->delay)
		free( data->delay);

	free( data);	
}
