#ifndef COLOR_CONVERSIONS_H
#define COLOR_CONVERSIONS_H

/***************************************************
 *  Name:        convertRGBtoXYZ
 *
 *  Returns:     Nothing
 *
 *  Parameters:  RGB values and XYZ references
 *
 *  Description: Given a color in RGB values converts
 *               to the corresponding XYZ values
 *
 ***************************************************/
void convertRGBtoXYZ(int inR, int inG, int inB, float * outX, float * outY, float * outZ);

/***************************************************
 *  Name:        convertXYZtoLab
 *
 *  Returns:     Nothing
 *
 *  Parameters:  XYZ values and Lab references
 *
 *  Description: Given a color in XYZ values converts
 *               to the corresponding Lab values
 *
 ***************************************************/
void convertXYZtoLab(float inX, float inY, float inZ, float * outL, float * outa, float * outb);

/***************************************************
 *  Name:        convertLabtoXYZ
 *
 *  Returns:     Nothing
 *
 *  Parameters:  Lab values and XYZ references
 *
 *  Description: Given a color in Lab values converts
 *               to the corresponding XYZ values
 *
 ***************************************************/
void convertLabtoXYZ( float inL, float ina, float  inb, float * outX, float * outY, float * outZ);

/***************************************************
 *  Name:        convertXYZtoRGB
 *
 *  Returns:     Nothing
 *
 *  Parameters:  XYZ values and RGB references
 *
 *  Description: Given a color in XYZ values converts
 *               to the corresponding RGB values
 *
 ***************************************************/
void convertXYZtoRGB(float inX, float inY, float inZ, int * outR, int * outG, int * outB);

/***************************************************
 *  Name:        Lab_color_difference_CIE76
 *
 *  Returns:     float
 *
 *  Parameters:  2 Lab color values
 *
 *  Description: Calculates and returns the difference
 *				 between 2 Lab colors based on the CIE76 formula
 *
 ***************************************************/
float Lab_color_difference_CIE76( float inL1, float ina1, float  inb1, float inL2, float ina2, float inb2);

/***************************************************
 *  Name:        RGB_color_Lab_difference_CIE76
 *
 *  Returns:     float
 *
 *  Parameters:  2 RGB color values
 *
 *  Description: Converts RGB values to Lab values and returns
 *				 the difference between 2 Lab colors based on
 *				 the CIE76 formula
 *
 ***************************************************/
float RGB_color_Lab_difference_CIE76( int R1, int G1, int B1, int R2, int G2, int B2);

/***************************************************
 *  Name:        Lab_color_difference_CIE94
 *
 *  Returns:     float
 *
 *  Parameters:  2 Lab color values
 *
 *  Description: Calculates and returns the difference
 *				 between 2 Lab colors based on the CIE94 formula
 *
 ***************************************************/
float Lab_color_difference_CIE94( float inL1, float ina1, float  inb1, float inL2, float ina2, float inb2);

/***************************************************
 *  Name:        RGB_color_Lab_difference_CIE94
 *
 *  Returns:     float
 *
 *  Parameters:  2 RGB color values
 *
 *  Description: Converts RGB values to Lab values and returns
 *				 the difference between 2 Lab colors based on
 *				 the CIE94 formula
 *
 ***************************************************/
float RGB_color_Lab_difference_CIE94( int R1, int G1, int B1, int R2, int G2, int B2);



float Lab_color_difference_CIEDE2000( float inL1, float ina1, float  inb1, float inL2, float ina2, float inb2);
float RGB_color_Lab_difference_CIEDE2000( int R1, int G1, int B1, int R2, int G2, int B2);


#endif // COLOR_CONVERSIONS_H
