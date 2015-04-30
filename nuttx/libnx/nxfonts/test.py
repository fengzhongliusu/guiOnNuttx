row = 16
col = 1
count = 1
num = 0
with open("nxfonts_bitmaps_sans28x37.c","a") as fw:
    while num < 6768:
        h = 0xa0 + row
        l = 0xa0 + col
        data = (h<<8) + l
        fw.write("    { {4,30,32,3,12,0}, l_cn_%x },\n" % data);
        count += 1
        if(count > 94):
            count = 1
            row += 1 
            col = 1
        else:
            col += 1
        num += 1
