#include <stdio.h>
#include <stdlib.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <emmintrin.h>

typedef union um128 {
    __m128i mm;
    long long lli[2];
    unsigned long long ulli[2];
    int si[4];
    unsigned int usi[4];
    short hi[8];
    unsigned short uhi[8];
    signed char qi[16];
    unsigned char uqi[16];
} __attribute__((aligned(16))) um128;


// OPENCV ESTÁ BIEN
// http://docs.opencv.org/modules/highgui/doc/user_interface.html

uchar margen;
IplImage *cambiada = NULL;

void onMouse(int event, int x, int y, int flags, void* img)
{

    if( event == CV_EVENT_LBUTTONDOWN ){
        IplImage *original = (IplImage *) img;

        if( cambiada == NULL )
            cambiada = cvCloneImage(original);


        cvNamedWindow("Imagen resultante",1);

        // Punto exacto seleccionado
        unsigned char *ptrSeleccionU = (unsigned char*) (( (original->imageData + ( y * original->widthStep )) + (x*4) ) );

        //printf("Las componentes de color son: Azul %d, Verde %d, Rojo %d \n",
        //                 *ptrSeleccionU, *(ptrSeleccionU+1), *(ptrSeleccionU+2));

        char azulSel = ((char)*(ptrSeleccionU))+0x80;
        char verdeSel = ((char)*(ptrSeleccionU+1))+0x80;
        char rojoSel = ((char)*(ptrSeleccionU+2))+0x80;

        /// variables necesarias
        __m128i *pOriginal, *pModificada;
        __m128i rangoSSE, pixelSeleccionadoSSE,
                mayorMin, menorMax, minEq, maxEq, blancoyNegro, dentroDeRango;
        um128 pixelSeleccionadoSSEmax, pixelSeleccionadoSSEmin, pixelesActuales;
        // arreglo para la comparación porque la comparación por ints no me funciona bien
        __m128i pixelEnteroSi = _mm_setr_epi8 ( 0xff,0xff,0xff,0,
                                                0xff,0xff,0xff,0,
                                                0xff,0xff,0xff,0,
                                                0xff,0xff,0xff,0);



        pOriginal = (__m128i *) original->imageData;
        pModificada = (__m128i *) cambiada->imageData;

        rangoSSE = _mm_set1_epi8((char)margen);

        // llenamos el registro SSE con el pixel seleccionado (cuatro veces)
        pixelSeleccionadoSSE = _mm_setr_epi8 ( azulSel, verdeSel, rojoSel, 0,
                                               azulSel, verdeSel, rojoSel, 0,
                                               azulSel, verdeSel, rojoSel, 0,
                                               azulSel, verdeSel, rojoSel, 0);

        pixelSeleccionadoSSEmin.mm = _mm_subs_epi8(pixelSeleccionadoSSE, rangoSSE);
        // canal alfa
        pixelSeleccionadoSSEmin.qi[3] = 0;
        pixelSeleccionadoSSEmin.qi[7] = 0;
        pixelSeleccionadoSSEmin.qi[11] = 0;
        pixelSeleccionadoSSEmin.qi[15] = 0;
        pixelSeleccionadoSSEmax.mm = _mm_adds_epi8(pixelSeleccionadoSSE, rangoSSE);
        // canal alfa
        pixelSeleccionadoSSEmax.qi[3] = 0;
        pixelSeleccionadoSSEmax.qi[7] = 0;
        pixelSeleccionadoSSEmax.qi[11] = 0;
        pixelSeleccionadoSSEmax.qi[15] = 0;


        int i, j;
        for ( i = 0; i < (original->imageSize)/16; i++){
            pixelesActuales.mm = _mm_load_si128(pOriginal++);
            
            // componentes de los pixeles actuales para pasar a blanco y negro antes de modificar los pixeles actuales
            unsigned char primerPixel = (unsigned char) (0.114 * (pixelesActuales.uqi[0]))
                                      + (unsigned char) (0.587 * (pixelesActuales.uqi[1]))
                                      + (unsigned char) (0.299 * (pixelesActuales.uqi[2]));
            unsigned char segundoPixel = (unsigned char)(0.114 * (pixelesActuales.uqi[4]))
                                       + (unsigned char) (0.587 * (pixelesActuales.uqi[5]))
                                       + (unsigned char) (0.299 * (pixelesActuales.uqi[6]));
            unsigned char tercerPixel = (unsigned char) (0.114 * (pixelesActuales.uqi[8]))
                                      + (unsigned char) (0.587 * (pixelesActuales.uqi[9]))
                                      + (unsigned char) (0.299 * (pixelesActuales.uqi[10]));
            unsigned char cuartoPixel = (unsigned char) (0.114 * (pixelesActuales.uqi[12]))
                                      + (unsigned char) (0.587 * (pixelesActuales.uqi[13]))
                                      + (unsigned char) (0.299 * (pixelesActuales.uqi[14]));
            // realizamos este cambio para que las comparaciones
            // con tipo de dato char funcionen correctamente
            // dato char + 0x80 , despues de sumar tenemos... 80h + 80h (tamaño byte) -> 0h
            //                                                256 + 80h -> 7Fh ->127 mayor número representable
            //                                                0   + 80h -> 80h -> -128 menor número representable
            //  80h     81h ---- 0h ---- 7Eh    7Fh
            //   |       |       |        |      |
            // -128    -127 ---- 0  ---- 126    127
            pixelesActuales.mm = _mm_setr_epi8(pixelesActuales.qi[0]+0x80, pixelesActuales.qi[1]+0x80,
                                               pixelesActuales.qi[2]+0x80, 0,
                                               pixelesActuales.qi[4]+0x80, pixelesActuales.qi[5]+0x80,
                                               pixelesActuales.qi[6]+0x80, 0,
                                               pixelesActuales.qi[8]+0x80, pixelesActuales.qi[9]+0x80,
                                               pixelesActuales.qi[10]+0x80, 0,
                                               pixelesActuales.qi[12]+0x80, pixelesActuales.qi[13]+0x80,
                                               pixelesActuales.qi[14]+0x80, 0);
            // hago esto porque el _mm_cmpgt_epi32 NO FUNCIONA
            // comparo primero por Componentes de color
            mayorMin = _mm_cmpgt_epi8( pixelesActuales.mm, pixelSeleccionadoSSEmin.mm );
            // ahora comparo el pixel entero
            /// SI FF FF FF FF NO 00 00 00 00
            mayorMin = _mm_cmpeq_epi32( mayorMin, pixelEnteroSi );
            // comparo primero por Componentes de color
            menorMax = _mm_cmplt_epi8( pixelesActuales.mm , pixelSeleccionadoSSEmax.mm);
            // ahora comparo el pixel entero
            /// SI FF FF FF FF NO 00 00 00 00
            menorMax = _mm_cmpeq_epi32(menorMax, pixelEnteroSi);
            
            minEq = _mm_cmpeq_epi32( pixelesActuales.mm, pixelSeleccionadoSSEmin.mm );
            maxEq = _mm_cmpeq_epi32( pixelesActuales.mm, pixelSeleccionadoSSEmax.mm );

            dentroDeRango = _mm_and_si128( mayorMin, menorMax ); // mayor que limite inferior y menor que limite superior
            dentroDeRango = _mm_or_si128 (dentroDeRango, minEq); // o igual al minimo
            dentroDeRango = _mm_or_si128 (dentroDeRango, maxEq); // o igual al maximo
            // fin creacion mascara
            
            /// si esta dentro de rango -> color original
            pixelesActuales.mm = _mm_and_si128( pixelesActuales.mm, dentroDeRango );
            // tenemos que hacer también la conversión de uchar a char porque la final volvemos a hacerlo para los pixeles actuales.
            blancoyNegro = _mm_setr_epi8(primerPixel+0x80, primerPixel+0x80, primerPixel+0x80, 0,
                                         segundoPixel+0x80, segundoPixel+0x80, segundoPixel+0x80, 0,
                                         tercerPixel+0x80, tercerPixel+0x80, tercerPixel+0x80, 0,
                                         cuartoPixel+0x80, cuartoPixel+0x80, cuartoPixel+0x80, 0);
            blancoyNegro = _mm_andnot_si128(dentroDeRango,blancoyNegro);
            
            // finalmente unimos para que salga los que no entrar en blanco y negro con los que si en su color original
            pixelesActuales.mm = _mm_or_si128( pixelesActuales.mm,  blancoyNegro);
            // revertimos el cambio sumando de nuevo 0x80
            // char + 0x80
            // para que cuando almacenemos en memoria los 4 pixeles,
            // vayan el el formato correcto.
            pixelesActuales.mm = _mm_setr_epi8((pixelesActuales.qi[0]+0x80), (pixelesActuales.qi[1]+0x80),
                                               (pixelesActuales.qi[2]+0x80), 0,
                                               (pixelesActuales.qi[4]+0x80), (pixelesActuales.qi[5]+0x80),
                                               (pixelesActuales.qi[6]+0x80), 0,
                                               (pixelesActuales.qi[8]+0x80), (pixelesActuales.qi[9]+0x80),
                                               (pixelesActuales.qi[10]+0x80), 0,
                                               (pixelesActuales.qi[12]+0x80), (pixelesActuales.qi[13]+0x80),
                                               (pixelesActuales.qi[14]+0x80), 0);
            _mm_store_si128(pModificada++, pixelesActuales.mm);
        }
        cvShowImage("Imagen resultante", cambiada);
        return;
    }

}

int main(int argc, char** argv) {

    if (argc != 2) {
        printf("Usage: %s image_file_name\n", argv[0]);
        return EXIT_FAILURE;
    }

    IplImage *imagenEntrada = cvLoadImage(argv[1], CV_LOAD_IMAGE_UNCHANGED);
    IplImage *imagenEntrada4c = cvCreateImage(cvGetSize(imagenEntrada), IPL_DEPTH_8U, 4);
    // Always check if the program can find a file
    if (!imagenEntrada) {
        printf("Error: fichero %s no leido\n", argv[1]);
        return EXIT_FAILURE;
    }
    unsigned char *pImg   = (unsigned char*) imagenEntrada->imageData;
    unsigned char *pImg4c = (unsigned char*) imagenEntrada4c->imageData;

    // pasamos la imagen a 4 canales para facilitar el trabajo con SSE y quepan justo 4 píxeles por registro
    int cont;
    for (cont = 0; cont < imagenEntrada->imageSize; cont += 3) {
        *pImg4c++ = *pImg++;
        *pImg4c++ = *pImg++;
        *pImg4c++ = *pImg++;
        *pImg4c++ = 0;
    }

    int rango;
    do{
        printf("Introduce el rango de oscilacion de color sobre el punto que selecciones"
                "\n(número entero entero entre 0 y 255): ");
        scanf("%i", &rango);
        
    }while( rango < 0 || rango > 255);
    margen = (uchar) rango;

    printf("Pulsa enter cuando quieras terminar la selección \n ");
    // a visualization window is created with title 'image'
    cvNamedWindow("Imagen Original", 1);
    cvShowImage("Imagen Original", imagenEntrada4c);
    // Se crea la ventana con la imagen original y se carga
    cvSetMouseCallback("Imagen Original", onMouse, (void *) imagenEntrada4c);
    // A partir de este momento se recogen los eventos que el usuario
    // realiza con el ratón

    cvWaitKey(0);
    // Una vez ha terminado la edición eliminamos las ventanas y liberamos de
    // memoria la imagen original.

    cvReleaseImage(&imagenEntrada);
    cvReleaseImage(&imagenEntrada4c);
    cvDestroyWindow("Imagen Original");
    cvDestroyWindow("Imagen resultante");

    // Preguntamos al uguario si quiere guardar la imagen modificada
    printf("Si quieres guardar pulsa 1 si no, pulsa otra tecla.  \n");
    int opt = 0;
    scanf("%d", &opt);
    if( opt == 1 ){
        cvSaveImage("resultado.jpg", cambiada, 0 );
        printf("Tu foto con un solo color se ha guardado con éxito\n");
    }else{
        printf("Has elegido no guardar\n");
    }

    // Una vez haya elegido si guardar o no la imagen modificada ya podemos
    // liberar la memoria que ésta ocupaba.
    cvReleaseImage(&cambiada);



    return EXIT_SUCCESS;
}