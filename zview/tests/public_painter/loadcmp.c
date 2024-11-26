load_cmp(obj)
int obj;
{
	int r = 0,
		o = 0,
		id,
		over = 0;
	char *t;
	unsigned char ct[2];
	GRECT b;
	char s[200];
	long len;

	xywh_to_area(0, 0, 640, 400, &b);
	t = file_tree[obj].ob_spec;
	ext(f_return, t);
	ext(f_name, t);
	id = file_len(f_return, &img_len);
	if (id)
	{
		id = Fopen(f_return, 0);
		if (id > 0)
		{
			Fread(id, 2L, ct);
			switch (ct[1])
			{
			case DOO:
				break;
			case DIN:
				b.g_h *= 2;
				break;
			case FAT:
				b.g_h *= 2;
				b.g_w *= 2;
				break;
			case PI3:
				o = 34;
				break;
			case PIC:
				o = 128;
				break;
			case DIN2:
				b.g_h *= 2;
				o = 34;
				break;
			default:
				sprintf(s, "[2][WRONG TYPE=%d][CANCEL]", (int) ct[1]);
				form_alert(1, s);
				return 0;
			}
			len = (long) b.g_h * (long) (b.g_w / 8L) * (long) planes + (long) o;
			if (img_len > (maxroom + maxroom))
				over = id;
			if (open_window(1, 0, o, 0, &b))
			{
				img_red = Fread(id, maxroom + maxroom, backbp);
				decode_cmp(over, ct[0], img_len);
				if (ani)
					graf_growbox(0, 0, 0, 0, b.g_x, b.g_y, b.g_w, b.g_h);
				wind_open(hdl, b.g_x, b.g_y, b.g_w, b.g_h);
				wind_slider(3);
				wd[hdl].open = 1;
				wd[hdl].type = ct[1];
				wd[hdl].off = o;
				wd[hdl].len = len;
				test_redraw();
				count++;
				picmenu(1);
				r = 1;
			}
			Fclose(id);
		}
	}
	return r;
}

decode_cmp(id, minimum, length)
int id;
unsigned char minimum;
long length;
{
	register int j;
	unsigned char val,
	 times,
	 sign;
	long i = 0L;

	scan = (unsigned char *) backbp;
	start = (unsigned char *) wd[hdl].bp;
	while (i <= length)
	{
		if (id)
			id = more_img(id);
		val = *scan++;
		i++;
		if (val == minimum)
		{
			times = *scan++;
			i++;
			val = *scan++;
			i++;
			for (j = times; j >= 0; j--)
				*start++ = val;
		} else
			*start++ = val;
	}
}
