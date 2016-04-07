#include <stdio.h>
#include <stdlib.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>



// OPENCV ESTÁ BIEN
// http://docs.opencv.org/modules/highgui/doc/user_interface.html

uchar margen;
IplImage *cambiada = NULL;



void onMouse(int event, int x, int y, int flags, void* img)
{

    if( event == CV_EVENT_LBUTTONDOWN ){

        // casteamos el puntero void* img a IplImage para poder tratarla dentro de la función onMouse
        IplImage *original = (IplImage *) img;

        // Comprobamos que la imagen "cambiada" no ha sido generada ya
        // ( para que no se genere cada vez que hacemos click )
        if( cambiada == NULL )
            cambiada = cvCloneImage(original);


        

        // Tenemos el pixel exacto para poder coger sus componentes de color.
        unsigned char *ptrSeleccionU = (unsigned char*) (( (original->imageData + ( y * original->widthStep )) + (x*3) ) );

        unsigned char rojoSel = *(ptrSeleccionU+2);
        unsigned char verdeSel = *(ptrSeleccionU+1);
        unsigned char azulSel = *ptrSeleccionU;


        // restas y sumas con saturacion hechas con ifs
        uchar azulSelMenorSat = 0;
        uchar verdeSelMenorSat = 0;
        uchar rojoSelMenorSat = 0;
        uchar azulSelMayorSat = 255;
        uchar verdeSelMayorSat = 255;
        uchar rojoSelMayorSat = 255;

        if( (azulSel - margen) > 0 )
            azulSelMenorSat = (azulSel - margen);

        if( (verdeSel - margen) > 0 )
            verdeSelMenorSat = (verdeSel - margen);

        if( (rojoSel - margen) > 0 )
            rojoSelMenorSat = (rojoSel - margen);

        if( (azulSel + margen) < 255 )
            azulSelMayorSat = (azulSel + margen);

        if( (verdeSel + margen) < 255 )
            verdeSelMayorSat = (verdeSel + margen);

        if( (rojoSel + margen) < 255 )
            rojoSelMayorSat = (rojoSel + margen);


        int i, j;
        for ( i = 0; i < original->height; i++ ) {
            uchar *ptrOrg  = (uchar*) ( original->imageData +
                                      ( i * original->widthStep ) );
            uchar *ptrCamb = (uchar*) ( cambiada->imageData +
                                      ( i * cambiada->widthStep ) );

            for (j = 0; j < original->width; j++) {
                uchar blue  = *ptrOrg++;
                uchar green = *ptrOrg++;
                uchar red   = *ptrOrg++;

                if( ( blue  >= azulSelMenorSat  && blue  <= azulSelMayorSat  ) &&
                    ( green >= verdeSelMenorSat && green <= verdeSelMayorSat ) &&
                    ( red   >= rojoSelMenorSat  && red   <= rojoSelMayorSat  ) )
                {
                    // si está dentro del rango, queda como antes.
                    *ptrCamb++ = blue;
                    *ptrCamb++ = green;
                    *ptrCamb++ = red;
                }else{
                    // si no está dentro del rango, en blanco y negro.
                    unsigned char ByN = (unsigned char) (0.114 * (blue))
                                      + (unsigned char) (0.587 * (green))
                                      + (unsigned char) (0.299 * (red));
                    *ptrCamb++ = ByN;
                    *ptrCamb++ = ByN;
                    *ptrCamb++ = ByN;
                }
            }
        }
        cvNamedWindow("Imagen resultante",1);
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
    // Always check if the program can find a file
    if (!imagenEntrada) {
        printf("Error: fichero %s no leido\n", argv[1]);
        return EXIT_FAILURE;
    }

    int rango;
    do{
        printf("Introduce el rango de oscilacion de color sobre el punto que selecciones"
                "\n(número entero entero entre 0 y 255): ");
        scanf("%i", &rango);

    }while( rango < 0 || rango > 255);
    margen = (uchar) rango;

    printf("Pulsa enter cuando quieras terminar la edición \n ");
    
    cvNamedWindow("Imagen Original", 1);
    cvShowImage("Imagen Original", imagenEntrada);
    
    // Se crea la ventana con la imagen original y se carga
    cvSetMouseCallback("Imagen Original", onMouse, (void *) imagenEntrada);
    // A partir de este momento se recogen los eventos que el usuario
    // realiza con el ratón

    cvWaitKey(0);
    // Una vez ha terminado la edición eliminamos las ventanas y liberamos de
    // memoria la imagen original.
    cvReleaseImage(&imagenEntrada);
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