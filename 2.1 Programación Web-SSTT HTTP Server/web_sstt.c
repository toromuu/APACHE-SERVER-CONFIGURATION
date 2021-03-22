#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/stat.h>
#include <time.h>
#include <regex.h>

#define VERSION		24
#define BUFSIZE		8096
#define ERROR		42
#define LOG		44

#define BADREQUEST	400
#define PROHIBIDO	403
#define NOENCONTRADO	404
#define UNSUPPORTED	415
#define MAXPETICIONES	429


#define NMAXACCESOS	10
#define ruta_MAX        4096
#define SEGUNDOSCOOKIE  30


/*
Son 10 tipos de extensiones diferentes, usaremos esta estructura para comprobar si el fichero tiene un tipo valido. Anadir "." para comprobar.
*/
struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{".gif", "image/gif" },
	{".jpg", "image/jpg" },
	{".jpeg","image/jpeg"},
	{".png", "image/png" },
	{".ico", "image/ico" },
	{".zip", "image/zip" },
	{".gz",  "image/gz"  },
	{".tar", "image/tar" },
	{".htm", "text/html" },
	{".html","text/html" },
	{0,0} };



void debug(int log_message_type, char *message, char *additional_info, int socket_fd) {

//400 BAD REQUEST
char buffer400cabecera[] = "HTTP/1.1 400 Bad request\r\nServer: Apache\r\nConection: close\r\nContent-Length: 354\r\nContent-Type: text/html\r\n";

char buffer400Htmlpage[] = "\r\n<p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><h1 style=\"text-align: center;\"><span style=\"color: #993300;\">400 Bad Request</span>.</h1><h1 style=\"text-align: center;\">That's an <span style=\"color: #993300;\">Error</span>.</h1><p style=\"text-align: center;\"><strong>La solicitud esta mal formada.</strong></p>";

//403 FORBIDDEN
char buffer403cabecera[] = "HTTP/1.1 403 Forbidden\r\nServer: Apache\r\nConection: close\r\nContent-Length: 374\r\nContent-Type: text/html\r\n";

char buffer403Htmlpage[] = "\r\n<p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><h1 style=\"text-align: center;\"><span style=\"color: #993300;\">403 Forbidden</span>.</h1><h1 style=\"text-align: center;\">That's an <span style=\"color: #993300;\">Error</span>.</h1><p style=\"text-align: center;\"><strong>La solicitud ha sido denegada por falta de permisos.</strong></p>";

//404 FILE NOT FOUND
char buffer404cabecera[] = "HTTP/1.1 404 Not Found\r\nServer: Apache\r\nConection: close\r\nContent-Length: 388\r\nContent-Type: text/html\r\n";

char buffer404Htmlpage[] = "\r\n<p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><h1 style=\"text-align: center;\"><span style=\"color: #993300;\">404 File Not Found</span>.</h1><h1 style=\"text-align: center;\">That's an <span style=\"color: #993300;\">Error</span>.</h1><p style=\"text-align: center;\"><strong>El archivo solicitado o url no se encuentra en este servidor.</strong></p>";


//415 UNSUPPORTED MEDIA TYPE

char buffer415cabecera[] = "HTTP/1.1 415 Unsupported Media Type\r\nServer: Apache\r\nConection: close\r\nContent-Length: 388\r\nContent-Type: text/html\r\n";

char buffer415Htmlpage[] = "\r\n<p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><h1 style=\"text-align: center;\"><span style=\"color: #993300;\">415 Unsupported Media Type</span>.</h1><h1 style=\"text-align: center;\">That's an <span style=\"color: #993300;\">Error</span>.</h1><p style=\"text-align: center;\"><strong>La extesion del recurso solicitado no esta soportada.</strong></p>";

//429 TOO MANY REQUEST

char buffer429cabecera[] = "HTTP/1.1 429 Too Many Requests\r\nServer: Apache\r\nConection: close\r\nContent-Length: 400\r\nContent-Type: text/html\r\n";

char buffer429Htmlpage[] = "\r\n<p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><h1 style=\"text-align: center;\"><span style=\"color: #993300;\">429 Too Many Requests</span>.</h1><h1 style=\"text-align: center;\">That's an <span style=\"color: #993300;\">Error</span>.</h1><p style=\"text-align: center;\"><strong>Ha excedido el numero maximo de peticiones al servidor en un instante.</strong></p>";

	
	//Fecha Actual

	time_t FechActual = time(NULL);
	struct tm *FechActual_tm;
	FechActual_tm = gmtime(&FechActual);
	char bufferFechActual[100] = { 0 };
	strftime(bufferFechActual,100, "Date: %a, %d %b %Y %T %Z\r\n", FechActual_tm);
	
	int fd ;
	char logbuffer[BUFSIZE*2];

	switch (log_message_type) {

		case ERROR:
			(void)sprintf(logbuffer,"ERROR: %s:%s Errno=%d exiting pid=%d",message, additional_info, errno,getpid());
			break;

		case BADREQUEST:// Enviar como respuesta 400 BAD REQUEST
			 //Una petición con un método HTTP no válido (i.e. GOT, PUST, etc.)
			
			strcat(buffer400cabecera, bufferFechActual);
			strcat(buffer400cabecera, buffer400Htmlpage);

			write(socket_fd,buffer400cabecera,strlen(buffer400cabecera));

			(void)sprintf(logbuffer,"BAD REQUEST: %s:%s",message, additional_info);
		break;

		case PROHIBIDO:// Enviar como respuesta 403 Forbidden
			//Petición a un recurso no permitido (i.e. ../../etc/passwd)

			strcat(buffer403cabecera, bufferFechActual);
			strcat(buffer403cabecera, buffer403Htmlpage);

			write(socket_fd,buffer403cabecera,strlen(buffer403cabecera));

			(void)sprintf(logbuffer,"FORBIDDEN: %s:%s",message, additional_info);
		break;


		case NOENCONTRADO:// Enviar como respuesta 404 Not Found
			// Fichero solicitado no ha sido encontrado
			strcat(buffer404cabecera, bufferFechActual);
			strcat(buffer404cabecera, buffer404Htmlpage);

			write(socket_fd,buffer404cabecera,strlen(buffer404cabecera));

			(void)sprintf(logbuffer,"NOT FOUND: %s:%s",message, additional_info);
		break;

		case UNSUPPORTED:// Enviar como respuesta 400 BAD REQUEST
			//Petición a un recurso con una extensión no válida (i.e. /imagen.zzz) 
			strcat(buffer415cabecera, bufferFechActual);
			strcat(buffer415cabecera, buffer415Htmlpage);

			write(socket_fd,buffer415cabecera,strlen(buffer415cabecera));

			(void)sprintf(logbuffer,"UNSUPPORTED FILE EXTENSION: %s:%s",message, additional_info);
		break;


		case MAXPETICIONES: //Enviar como respuesta 429 Too Many Requests
			// Expiración por demasiadas solicitudes, cookies==10
			strcat(buffer429cabecera, bufferFechActual);
			strcat(buffer429cabecera, buffer429Htmlpage);
			write(socket_fd,buffer429cabecera,strlen(buffer429cabecera));

			(void)sprintf(logbuffer,"TOO MANY REQUESTS: %s:%s",message, additional_info);
		break;


		case LOG: (void)sprintf(logbuffer," INFO: %s:%s:%d",message, additional_info, socket_fd);
			break;
	}

	if((fd = open("webserver.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0) {
		(void)write(fd,logbuffer,strlen(logbuffer));
		(void)write(fd,"\n",1);
		(void)close(fd);
	}

	if(log_message_type == ERROR || log_message_type == NOENCONTRADO || log_message_type == PROHIBIDO || log_message_type == UNSUPPORTED || log_message_type == MAXPETICIONES || log_message_type ==  BADREQUEST) {
		close(socket_fd);
		exit(6);
	}
}




//Como funciona select http://manpages.ubuntu.com/manpages/bionic/es/man2/select_tut.2.html

/*

Cada vez que llega una solicitud de conexión TCP de un cliente, se abrirá un socket 
(el cual tendrá un descriptor de fichero asociado) y un proceso hijo se quedará escuchando a la espera de datos disponibles (de ese mismo cliente) antes de cerrar dicha conexión TCP. 
 
Mantener esta conexión TCP abierta se lleva a cabo en la función Persistencia , donde es la función select la encargada de gestionar la concurrencia entre varios descriptores al mismo tiempo.
Bloqueando el proceso mientras  se espera , hasta un máximo de 10 segundos , un "cambio de estado" el cual se produce cuando vuelven a haber datos disponibles en el descriptor de fichero asociado al socket.

Es decir, la conexión TCP permanece abierta, mientras el proceso siga teniendo la exclusión mutua al descriptor de fichero asociado al socket, y por tanto mientras el proceso no termine porque el select continua detectando datos en el descriptor de fichero por el hecho de que el cliente continúa realizando nuevas peticiones las cuales reinician la espera-timeout del select a 10 segundos cada vez.

*/

int persistencia(int descriptorFichero, long int segPersistencia, long int microsegComprobacion ) {

	fd_set conjuntoDescriptoresPersistencia; // Se define un nuevo conjunto de descriptores que escuchara select
	FD_ZERO(&conjuntoDescriptoresPersistencia); // Se limpia el conjunto
	FD_SET(descriptorFichero, &conjuntoDescriptoresPersistencia); // Se añade el descriptor asociado al socket al conjunto

	// Contador de tiempo 
	struct timeval tiempo;//Etructura time
	tiempo.tv_sec = segPersistencia; // Segundos
        tiempo.tv_usec = microsegComprobacion; // MicroSegundos
	
	// En caso de que hayan descriptores de fichero con datos disponibles durante el tiempo asignado devolvera el número de estos. En caso contrario devolvera 0, y en caso de error -1. 
	int perst = select(descriptorFichero+1,&conjuntoDescriptoresPersistencia, NULL, NULL,&tiempo);
	return perst;
}



void process_web_request(int descriptorFichero){


	while (persistencia(descriptorFichero,10,0)){ 	

		debug(LOG,"request","Ha llegado una peticion",descriptorFichero);

		//
		// Definir buffer y variables necesarias para leer las peticiones
		//
		char bufferSolicitud[BUFSIZE] = {0};
		char bufferRespuesta[BUFSIZE] = {0};
		int contadorCookie = 0;
		int nExtension = -1;

		//
		// Leer la petición HTTP.
		//

		int nBytesLeidos = read(descriptorFichero,bufferSolicitud,BUFSIZE);
		while (persistencia(descriptorFichero,0,190000)) {
			nBytesLeidos += read(descriptorFichero,bufferSolicitud +nBytesLeidos , BUFSIZE - nBytesLeidos );
		}
		
		//printf("%s\n", bufferSolicitud);


		//
		// Comprobación de errores de lectura
		//
		if (nBytesLeidos<0){
			debug(ERROR,"Error de lectura","socket", 0);
			//close(descriptorFichero);
		}

		//
		// Si la lectura tiene datos válidos terminar el buffer con un \0
		//

		strcat(bufferSolicitud,"\0");

		//
		// Decisión de diseño.
		// Se eliminan los caracteres de retorno de carro y nueva linea --> Podemos aprovecharlo y usar dichos caracteres como marcas para parsear la solicitud
		//


		// Tratamiento de la Solicitud.
		
		/*
		/ Strtok_r sirve para cortar una cadena por la primera coincidencia con una marca dada, en nuestro caso almacenamos dicha informacion hasta el corte en otra cadena
		/ El resto de la cadena que queda despues del corte, se queda apuntada por NULL, por lo que es de especial relevancia si vamos a volver a usar strtok_r para otros cortes
		/ guardar esta posicion de memoria.
		*/

		char *save0 = NULL;

		// Primera linea: Metodo URL Version
		char *lineaSolicitud = strtok_r(bufferSolicitud,"\r\n",&save0); 

		char Aux[BUFSIZE] = { 0 };
		strcpy(Aux, lineaSolicitud); // Dentro de esta linea, repetimos el proceso pero cortando por el limitador " "

			char *save1 = NULL;
			char *Metodo  = strtok_r(Aux,  " ",&save1);
			char *Url     = strtok_r(NULL, " ",&save1);
			char *Version = strtok_r(NULL, " ",&save1);
		
		// El resto de cabeceras
		// Como habiamos guardado la posicion de corte tras la primera linea, podemos obtener el resto de cabeceras de la solicitud
		char *cabecera = strtok_r(NULL, "\r\n",&save0);               


		// Comprobación del formato de la primera linea
		int erroPrimeraLinea;
		regex_t regex;
		//erroPrimeraLinea = regcomp(&regex, "[a-zA-Z]+ .* HTTP\\/1\\.\\d{1}", REG_EXTENDED);
		erroPrimeraLinea = regcomp(&regex, "[a-zA-Z]+ .* HTTP\\/(1\\.1)|(1\\.0)", REG_EXTENDED);
		erroPrimeraLinea = regexec(&regex, lineaSolicitud, 0, NULL, 0);

		if (erroPrimeraLinea) {	

			// Comprobacion de version correcta
			if ( (Version!=NULL) ) {
				if ((strncmp(Version,"HTTP/1.1",8)!=0) && (strncmp(Version,"HTTP/1.0",8)!=0) ) {
					debug(BADREQUEST,"Error 400","Version HTTP no disponible",descriptorFichero);
				}
			}
			
			debug(BADREQUEST,"Error 400","Peticion mal formulada no válido ",descriptorFichero);

		}
		

		//
		// TRATAR LOS CASOS DE LOS DIFERENTES METODOS QUE SE USAN
		//


		// TRATAMIENTO GET

		if (strcmp(Metodo,"GET")==0){

		  
			// Se crea la ruta que nos pide la Url
			struct stat urlStat;
			char ruta[ruta_MAX] = {0};
			strcat(ruta,".");// Necesario partir del directorio actual
			strcat(ruta,Url);
			int veri = stat(ruta,&urlStat); //Hay que cargar la estructura

			//
			// Como se trata el caso de acceso ilegal a directorios superiores de la
			// jerarquia de directorios del sistema 
			//

			/*
			/  Comprobar que no exista una coincidencia con /../
			/  La Url puede ser tanto un directorio como un archivo (la comprobacion access, no hace distincion solo comprueba permisos, obviamente si el fichero se encuentra en un directorio
			/ superior no contara con los permisos)
			*/

			
			//if( (strstr(Url, "/../")!= NULL) || (access(ruta,R_OK)<0) ) {
			if( (strstr(Url, "/../")!= NULL) ) {
				debug(PROHIBIDO,"Error 403","El archivo solicitado no ha sido leido por falta de permisos",descriptorFichero);
				//close(descriptorFichero);
			}

			
			//
			// Como se trata el caso excepcional de la URL que no apunta a ningún fichero html
			//

			/*
			/  Si se trata de un directorio hay que buscar el archivo index.html, dentro de el, para ello concatenamos a la url -> /index.html. 
			/  Luego comprobamos si existe dicho archivo en ese directorio
			*/

			if(S_ISDIR(urlStat.st_mode)){
		   		strcat(ruta,"index.html");
				//el fichero index tiene la extension html, que se corresponde con el valor 9 dentro de la estructura extensions
				nExtension=9;
		   		if ((stat(ruta,&urlStat))!=0){
					debug(NOENCONTRADO,"Error 404","El fichero index.html no existe en la direccion proporcionada",descriptorFichero);
					//close(descriptorFichero);
				}
				
			}else { //Si no es un directorio

				//
				// Evaluar el tipo de fichero que se está solicitando, y actuar en
				// consecuencia devolviendolo si se soporta u devolviendo el error correspondiente en otro caso
				//

				/*
				/  Si la url corresponde a un fichero y no a un directorio, tendremos que verificar que su extension se corresponde con alguna de
				/  las válidas definidas en la estructura extensions [] .Para ello cortamos por el "." , y recorremos la estructura buscando si alguna coincide, si coincide, nos interesa guardarla
				/  si no coincide, sera un error
				*/

				char *extension = strrchr(ruta,'.');
		 		

				for (int i = 0; i < 10; ++i){
				    	if(strcmp(extension, extensions[i].ext)==0){
				    	    nExtension = i;
				    	}
		  	    	}

				if (nExtension==-1){
					debug(UNSUPPORTED,"Error 415","Extension del fichero solicitado no soportado",descriptorFichero);
				}
			
			}
			 

			//Comprobamos que la direccion existe,(tanto si hace referencia a un fichero como un directorio) y de paso cargamos la estructura
			if(veri<0){
				debug(NOENCONTRADO,"Error 404","La direccion url proporcionada no existe",descriptorFichero);
			}


			//Tramiento de Cookies, solo cuando se envia el index.html
			char *indexFileAuxCookies = strrchr(ruta,'/');


			if(strcmp(indexFileAuxCookies, "/index.html")==0 ) {
			
				//Tramiento de Cookies
				while(cabecera){

					//Aprovechamos el recorrido de la cookie para la comprobacion de host
					if (strncmp(cabecera,"Host:",5)==0){

							char Aux2[BUFSIZE] = { 0 };
							strcpy(Aux2,cabecera);
							char *save2 = NULL;
							char *Header  = strtok_r(Aux2, " ",&save2);
							char *Host = strtok_r(NULL, "\r\n",&save2);
							//Tipo fichero no soportado
							if ( Header == NULL || Host == NULL ){
							    debug(BADREQUEST,"Error 400","Cabecera de Host mal formada",descriptorFichero);
							    //close(descriptorFichero);
							}else if ((strcmp(Host,"")==0 )||(strcmp(Header,"")==0) ){
								debug(BADREQUEST,"Error 400","Cabecera de Host mal formada",descriptorFichero);
							    	//close(descriptorFichero);
							}
							else if ((strcmp(Host,"www.sstt2188.org")!=0 )&&(strncmp(Host,"192.168.56.102",14)!=0) ){
						       		debug(BADREQUEST,"Error 400","Host erroneo",descriptorFichero);
								//close(descriptorFichero);
						      	}
				   	}

					// Extracción del valor de la cookie enviada en la solicitud
					if (strncmp(cabecera,"Cookie",6)==0){
						char Aux2[BUFSIZE] = { 0 };
						strcpy(Aux2,cabecera);
						char *save2 = NULL;
						char *Header  = strtok_r(Aux2, "=\r\n",&save2);
						char *nCookie = strtok_r(NULL, "=\r\n",&save2);
						contadorCookie = atoi(nCookie);
			   		}

			   	cabecera = strtok_r(NULL,"\r\n",&save0);
				}


			   	// Al décimo acceso se denegará el acceso al contenido

			   	if(contadorCookie >= NMAXACCESOS){
					debug(MAXPETICIONES,"Error 429 Too Many Requests","Numero cookies superadas",descriptorFichero);
					//close(descriptorFichero);
			   	}else contadorCookie++;

			}
			else{
				
				while(cabecera){
					// Comprobacion de la cabecera Host es correcta
					if (strncmp(cabecera,"Host:",5)==0){

							char Aux2[BUFSIZE] = { 0 };
							strcpy(Aux2,cabecera);
							char *save2 = NULL;
							char *Header  = strtok_r(Aux2, " ",&save2);
							char *Host = strtok_r(NULL, "\r\n",&save2);
							//Tipo fichero no soportado
							if ( Header == NULL || Host == NULL ){
							    debug(BADREQUEST,"Error 400","Cabecera de Host mal formada",descriptorFichero);
							    //close(descriptorFichero);
							}else if ((strcmp(Host,"")==0 )||(strcmp(Header,"")==0) ){
								debug(BADREQUEST,"Error 400","Cabecera de Host mal formada",descriptorFichero);
							    	//close(descriptorFichero);
							}
							else if ((strcmp(Host,"www.sstt2188.org")!=0 )&&(strncmp(Host,"192.168.56.102",14)!=0) ){
						       		debug(BADREQUEST,"Error 400","Host erroneo",descriptorFichero);
								//close(descriptorFichero);
						      	}
				   	}
				
				    // Avanzamos cortando la cabecera linea por linea
				    cabecera = strtok_r(NULL,"\r\n",&save0);
				}

			}

			//
			// En caso de que el fichero sea soportado, exista, etc. se envia el fichero con la cabecera
			// correspondiente, y el envio del fichero se hace en blockes de un máximo de  8kB
			//


			// Abrimos el fichero solicitado dada la url, en modo lectura
			FILE *f1 = fopen (ruta, "rb");
				if (f1!=NULL){

						/*
						/ Para construir el mensaje de respuesta, fijarse en el siguiente ejemplo:
						/
						/ linea estado     ->		             HTTP/1.1 200 OK\r\n
						/ Cabeceras utiles ->			     Server: Apache/2\r\n					     
						/			                     Set-Cookie: contadorCookie=0; Expires=Wed, 21 Oct 2015 07:28:00 GMT;\r\n
						/				             Content-Length: 9237\r\n
						/					     Content-Type: text/html; charset=utf-8\r\n
						/				             Connection: Keep-Alive\r\n
						/				             Keep-Alive: timeout=10, max=20\r\n
						/ Linea espacio en blanco ->                 \r\n
						/ Contenido del fichero solicitado->         Fichero html o imagen
						/
						*/


						// Construiremos el mensaje concatenando en el buffer de Respuestas todos los elementos de la solicitud


						// Linea de estado
						strcat(bufferRespuesta,Version);
						strcat(bufferRespuesta, " 200 OK\r\n");

						// Nombre del servidor
						strcat(bufferRespuesta, "Server: Apache\r\n");

						// Fecha
						time_t FechActual = time(NULL);
						struct tm *FechActual_tm;
						FechActual_tm = gmtime(&FechActual);
						char bufferFechActual[100] = { 0 };
						strftime(bufferFechActual,100, "Date: %a, %d %b %Y %T %Z\r\n", FechActual_tm);
						strcat(bufferRespuesta, bufferFechActual);

						// Cookie con la fecha de expiracion
						char Cookie[256] = { 0 };
		   				char bufferCookie[256] = { 0 };
						struct tm *FechExpiracion_tm;
						time_t FechExpiracion = FechActual + SEGUNDOSCOOKIE ; // le sumamos a la fecha actual 30 seg
						FechExpiracion_tm = gmtime(&FechExpiracion);
						sprintf(Cookie, "Set-Cookie: contadorCookie=%d; ", contadorCookie);
						strftime(bufferCookie, 256, "Expires=%a, %d %b %Y %T %Z",FechExpiracion_tm);
						strcat(Cookie, bufferCookie);
						strcat(bufferRespuesta, Cookie);
						strcat(bufferRespuesta, "\r\n");

						// Tamano total del fichero
						char tamanoFich[128] = { 0 };
						sprintf(tamanoFich, "Content-Length: %ld\r\n",urlStat.st_size);
						strcat(bufferRespuesta, tamanoFich);

						//Content-Type
						strcat(bufferRespuesta,"Content-Type: ");
						strcat(bufferRespuesta,extensions[nExtension].filetype);
						strcat(bufferRespuesta, "\r\n");


						//Keep-Alive
						strcat(bufferRespuesta,"Connection: Keep-Alive\r\nKeep-Alive: timeout=10, max=20\r\n");


						// Linea en blanco para separar las cabeceras de los datos a enviar
						strcat(bufferRespuesta, "\r\n");


						//printf("\n\n%s \n\n",bufferRespuesta);



						/*
						/  Se envia la cabecera correspondiente y el fichero solicitado con la restricción de que
						/  el envio del fichero se hace en blockes de un máximo de  8kB -> no podemos meterlo todo en el bufferRespuesta, asi que enviaremos
						/  primero la cabecera  y posteriormente el archivo por trozos.
						*/


						// ENVIO DE LA CABECERA

						size_t nBytesEscritos = write(descriptorFichero,bufferRespuesta,strlen(bufferRespuesta));

						 if ( strlen(bufferRespuesta) != nBytesEscritos) {
						nBytesEscritos += write(descriptorFichero, bufferRespuesta+nBytesEscritos, strlen(bufferRespuesta)-nBytesEscritos);
			       			 }


						// ENVIO DEL ARCHIVO

						char bufferFile[BUFSIZE] = { 0 };
						int readbytes;

						// Mientras no se llegue al final del archivo
						// La funcion fread mueve el puntero dentro del fichero con cada lectura de bytes
						while (!feof(f1)){
							char bufferFich[BUFSIZE] = { 0 };
							int nFichBytesLeidos = fread(bufferFich,1,BUFSIZE,f1); //Lee 8094 bloques de 1 byte de tamano, por lo que en bufferFich tendremos como maximo 8 Kib

							// Tratamiento si se produce algun error en la lectura
							if (ferror(f1)){
			 				   //printf("Ha ocurrido algún error en la lectura de números.\n");
							   //close(descriptorFichero);
							   debug(ERROR, "Read", "fatal file read", 0);
		  					}

							// Es interesante conocer si se ha llegado al final del fichero y ver los datos enviados
							/*if (feof(f1)) {
							    write(descriptorFichero,bufferFich,nFichBytesLeidos);
							    //printf("%s \n",bufferFich);


							}*/

							// Se envian los datos almacenados en el buffer,el cual tendra como maximo 8kib y como minimo dependera de la la division de bloques del fichero en bloques de 8kib, siendo el numero de bytes < 8kib en el ultimo envio
							else write(descriptorFichero,bufferFich,nFichBytesLeidos);


						}
						fclose(f1);
						//}

				 //Fichero no encontrado");
				}else debug(NOENCONTRADO,"Error 404","El fichero no existe",descriptorFichero);
		
		} //FIN DEL TRATAMIENTO DEL GET


		// TRATAMIENTO POST
		else if (strcmp(Metodo,"POST")==0){

			//Comprobacion que la URL es correcta, nos solicita una accion
			if (strcmp(Url,"/accion_form.html")!=0){
				debug(BADREQUEST,"Error 400","Accion no permitida",descriptorFichero);
			}

				
			/* Ejemplo de peticion Post

			POST / HTTP/1.1\r\n
			Host: www.sstt2188.org\r\n
			Content-Type: application/x-www-form-urlencoded\r\n
			Content-Length: 13\r\n
			.....................etc
			\r\n
			email=diegoalejandro.toror@um.com\r\n
			*/


			//
			// Debemos comprobar que el correo introducido en el formulario es correcto.
			// Dado que vamos a iterar la cabecera para comprobar errores, aprovecharemos para extraer dicho correo

			char *Valor=NULL;
		   
			//Tratamos iterativamente la cabecera para comprobar errores
			while(cabecera){

				// Comprobacion de la cabecera Host es correcta
				if (strncmp(cabecera,"Host:",5)==0){

						char Aux2[BUFSIZE] = { 0 };
						strcpy(Aux2,cabecera);
						char *save2 = NULL;
						char *Header  = strtok_r(Aux2, " ",&save2);
						char *Host = strtok_r(NULL, "\r\n",&save2);
						//Tipo fichero no soportado
						if ( Header == NULL || Host == NULL ){
						    debug(BADREQUEST,"Error 400","Cabecera de Host mal formada",descriptorFichero);
						}else if ((strcmp(Host,"")==0 )||(strcmp(Header,"")==0) ){
							debug(BADREQUEST,"Error 400","Cabecera de Host mal formada",descriptorFichero);
						}
						else if ((strcmp(Host,"www.sstt2188.org")!=0 )&&(strncmp(Host,"192.168.56.102",14)!=0) ){
					       		debug(BADREQUEST,"Error 400","Host erroneo",descriptorFichero);
					      	}
			   	}
				
				// Comprobacion de que el formato del formulario es correcto
				if (strncmp(cabecera,"Content-Type:",13)==0){

					char Aux2[BUFSIZE] = { 0 };
					strcpy(Aux2,cabecera);
					char *save2 = NULL;
					char *Header  = strtok_r(Aux2, " ",&save2);
					char *Type = strtok_r(NULL, "\r\n",&save2);
					//Tipo fichero no soportado
					
					if  (strncmp(Type,"application/x-www-form-urlencoded",33)!=0){
				       		debug(UNSUPPORTED,"Error 415","Extension del fichero solicitado no soportado",descriptorFichero);
				      	}
			   	}
				
				// Extraemos el valor de la clave email
				if (strncmp(cabecera,"email=",5)==0){

					char Aux2[BUFSIZE] = { 0 };
					strcpy(Aux2,cabecera);
					char *save2 = NULL;
					char *Header  = strtok_r(Aux2, "=",&save2);
					char *Value = strtok_r(NULL, "\r\n",&save2);
					Valor = Value;
			   	}
			    // Avanzamos cortando la cabecera linea por linea
			    cabecera = strtok_r(NULL,"\r\n",&save0);
			}

			// Formulacion de la cabecera de respuesta

			// Linea de estado
			strcat(bufferRespuesta,Version);
			strcat(bufferRespuesta, " 200 OK\r\n");
			
			// Nombre del servidor
			strcat(bufferRespuesta, "Server: Apache\r\n");
			
			// Fecha
			time_t FechActual = time(NULL);
			struct tm *FechActual_tm;
			FechActual_tm = gmtime(&FechActual);
			char bufferFechActual[100] = { 0 };
			strftime(bufferFechActual,100, "Date: %a, %d %b %Y %T %Z\r\n", FechActual_tm);
			strcat(bufferRespuesta, bufferFechActual);

			//Content-Type
			strcat(bufferRespuesta,"Content-Type: text/html\r\n");

			//Keep-Alive
			strcat(bufferRespuesta,"Connection: Keep-Alive\r\nKeep-Alive: timeout=10, max=20\r\n");
			
			//Expresion regular que valida el formato de los emails de la umu
			int reti;
			regex_t regex;
			reti = regcomp(&regex, "[a-zA-Z]+\\.[a-zA-Z]+\\%40um\\.es", REG_EXTENDED);
			reti = regexec(&regex, Valor, 0, NULL, 0);
			
			   if (!reti){
			    //if(strcmp(Valor,"diegoalejandro.toror@um.com")==0){
					char bufferPostOkayHtmlpage[] = "<p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><h1 style=\"text-align: center;\"><span style=\"color: #993300;\">Email Correcto </span>.</h1><h1 style=\"text-align: center;\">That's a <span style=\"color: #993300;\">Win</span>.</h1><p style=\"text-align: center;\"><strong>La solicitud Post esta bien formada.</strong></p>";
					// Tamano total del fichero
					char tamanoFich[128] = { 0 };
					sprintf(tamanoFich, "Content-Length: %ld\r\n",strlen(bufferPostOkayHtmlpage));
					strcat(bufferRespuesta, tamanoFich);

					//strcat(bufferRespuesta, "Content-Length: 353\r\n");

					// Linea en blanco para separar las cabeceras de los datos a enviar
					strcat(bufferRespuesta, "\r\n");

					// ENVIO DE LA CABECERA
					write(descriptorFichero,bufferRespuesta,strlen(bufferRespuesta));
					
					// ENVIO DEL FICHERO HTML
					write(descriptorFichero,bufferPostOkayHtmlpage,strlen(bufferPostOkayHtmlpage));	
			    }
			    else {
					char bufferPostNokayHtmlpage[] = "<p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><h1 style=\"text-align: center;\"><span style=\"color: #993300;\">Email Incorrecto </span>.</h1><h1 style=\"text-align: center;\">That's a <span style=\"color: #993300;\">Fail</span>.</h1><p style=\"text-align: center;\"><strong>La solicitud Post esta bien formada, pero el correo introducido no.</strong></p>";

					// Tamano total del fichero
					char tamanoFich[128] = { 0 };
					sprintf(tamanoFich, "Content-Length: %ld\r\n",strlen(bufferPostNokayHtmlpage));
					strcat(bufferRespuesta, tamanoFich);
					//strcat(bufferRespuesta, "Content-Length: 387\r\n");

					// Linea en blanco para separar las cabeceras de los datos a enviar
					strcat(bufferRespuesta, "\r\n");

					// ENVIO DE LA CABECERA
					write(descriptorFichero,bufferRespuesta,strlen(bufferRespuesta));

					// ENVIO DEL FICHERO HTML
					write(descriptorFichero,bufferPostNokayHtmlpage,strlen(bufferPostNokayHtmlpage));
				 }
				   
			} //FIN TRATAMIENTO DEL POST
			
		
		// Metodo No implementado o Sintaxis Incorreta
	 	else {  
			debug(UNSUPPORTED,"Error 415","Una petición con un método HTTP no válido (i.e. GOT, PUST, etc.)",descriptorFichero);
		}

    }//WHILE PERSISTENCIA

    close(descriptorFichero);
    exit(7);
}




int main(int argc, char **argv)
{
	int i, port, pid, listenfd, socketfd;
	socklen_t length;
	static struct sockaddr_in cli_addr;	// static = Inicializado con ceros
	static struct sockaddr_in serv_addr;	// static = Inicializado con ceros

	//  Argumentos que se esperan:
	//
	//  argv[1]
	//  En el primer argumento del programa se espera el puerto en el que el servidor escuchara
	//
	//  argv[2]
	//  En el segundo argumento del programa se espera el directorio en el que se encuentran los ficheros del servidor

	//
	//  Verificar que los argumentos que se pasan al iniciar el programa son los esperados
	//  ¿Comprobacion de tipos? -> en c no es posible de forma sencilla, no merece la pena solo para esta comprobación, si el tipo de datos es incorrecto se mostrara más adelante
	//
	if(argc != 3) {
		(void)printf("USAGE: ./web_sstt <puerto> <directorio_servidor>\n");
		exit(1);
	}

	//
	//  Verificar que el directorio escogido es apto. Que no es un directorio del sistema y que se tienen
	//  permisos para ser usado
	//

	struct stat directorio;

	//Se carga la estructura directorio con esta llamada a la funcion stat.
	if(stat(argv[2],&directorio)!=0){
		(void)printf("ERROR: Directorio no valido, no existe %s\n",argv[2]);
	    exit(2);
	}


	if (!S_ISDIR(directorio.st_mode)){
		(void)printf("ERROR: Directorio no valido %s\n",argv[2]);
	    exit(3);
      	}

	if(access(argv[2],W_OK)!=0||access(argv[2],R_OK)!=0){
		(void)printf("ERROR: Permiso de uso del directorio denegado %s\n", argv[2]);
		exit(4);
	}


	if(chdir(argv[2]) == -1){
		(void)printf("ERROR: No se puede cambiar de directorio %s\n",argv[2]);
		exit(5);
	}

	// Hacemos que el proceso sea un demonio sin hijos zombies
	if(fork() != 0)
		return 0; // El proceso padre devuelve un OK al shell

	(void)signal(SIGCHLD, SIG_IGN); // Ignoramos a los hijos
	(void)signal(SIGHUP, SIG_IGN); // Ignoramos cuelgues

	debug(LOG,"web server starting...", argv[1] ,getpid());

	/* setup the network socket */
	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0)
		debug(ERROR, "system call","socket",0);

	port = atoi(argv[1]);

	if(port < 0 || port >60000)
		debug(ERROR,"Puerto invalido, prueba un puerto de 1 a 60000",argv[1],0);

	/*Se crea una estructura para la información IP y puerto donde escucha el servidor*/
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); /*Escucha en cualquier IP disponible*/
	serv_addr.sin_port = htons(port); /*... en el puerto port especificado como parámetro*/

	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
		debug(ERROR,"system call","bind",0);

	if( listen(listenfd,64) <0)
		debug(ERROR,"system call","listen",0);

	while(1){
		length = sizeof(cli_addr);
		//Extrae la primera solicitud de conexión en la cola de conexiones pendientes para el socket de escucha, sockfd, crea un nuevo socket conectado y devuelve un nuevo descriptor de archivo que hace referencia a ese socket.
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
			debug(ERROR,"system call","accept",0);

		if((pid = fork()) < 0) {
			debug(ERROR,"system call","fork",0);
		}
		else { 
			if(pid == 0) { 	// Proceso hijo
				(void)close(listenfd);
				process_web_request(socketfd);

			} else { // Proceso padre
				(void)close(socketfd); 
			 }
		}//ELSE
	}//wHILE
}//MAIN











/* BIBLIOGRAFÍA Y FUENTES

persistencia:
	*Implementacion de la persistencia con select-> sesion1-sockets.pdf
							http://manpages.ubuntu.com/manpages/bionic/es/man2/select_tut.2.html -> uso del select y macros 
							http://www.chuidiang.org/clinux/sockets/socketselect.php -> como funciona select
							http://man7.org/linux/man-pages/man2/select.2.html -> estructura timeval

process_web_request:

	*read(int fd, void *buf, size_t count)
		http://man7.org/linux/man-pages/man2/read.2.html
		Operación read: Lee datos de un descriptor de fichero, en particular un socket.

	*size_t https://hackxcrack.net/foro/c/duda-con-size_t-en-c/


	*Comprobacion de errores de lectura

		http://man7.org/linux/man-pages/man2/read.2.html

		On error, -1 is returned, and errno is set appropriately.  In this
		case, it is left unspecified whether the file position (if any)
		changes.

		Usamos la funcion debug ya implementada en el código
		debug(int log_message_type, char *message, char *additional_info, int socket_fd)


		close (int filedes);
		Operación: Cierra un socket (o descriptor de fichero)
			Retorna: 0 con exito ; -1 indica error
			Filedes: socket que se tiene que cerrar


	*Recordar el uso de "Strings" en c y porque poner /0 al final de la cadena
	http://platea.pntic.mec.es/vgonzale/cyr_0204/cyr_01/control/lengua_C/cadenas.htm


	*Parseo de la primera linea de la peticion
	
	/ Strtok_r sirve para cortar una cadena por la primera coincidencia con una marca dada, en nuestro caso almacenamos dicha informacion hasta el corte en otra cadena
	/ El resto de la cadena se queda despues del corte queda apuntada por NULL, sin embargo es de especial relevancia si vamos a volver a usar strtok_r para otros cortes
	/ guardar esta posicion de memoria.
	

		Separación de los campos GET URL VERSION
		¿Porque usar strtok_r en vez de strtok?
		https://riptutorial.com/es/c/example/2557/tokenizacion--strtok-----strtok-r----y-strtok-s---


		Para el resto de las lineas de la cabecera.
		Recorremos linea por linea, buscando con la funcion strncmp si la cabecera es igual a
		"Cookie" o hasta llegar al final
		http://www.cplusplus.com/reference/cstring/strncmp/


	*Extraer el número de peticiones anteriores,observar el formato de las cookies
	https://cybmeta.com/que-son-las-cookies-y-como-funcionan

	*Conversion de string a entero
	http://www.cplusplus.com/reference/cstdlib/atoi/


	*La Url puede ser tanto un directorio como un archivo.
	Si es un directorio, hay que buscar el archivo index.html -> usar la comprobación con la estructura stat igual que en el main


	*Como cortar la url del ruta, por la extension -> con strrchr localiza un carácter en una cadena, buscando desde el final
	https://es.wikipedia.org/wiki/String.h

	*Remove de un caracter sencillo en c, -> buffer = buffer + 1
						 o anadir a la estructura extensions "." a las extensiones para realizar la comprobacion 

	*Ejemplo de linea solicitud
	//char bufferSolicitud[BUFSIZE] = {"GET /hello.htm HTTP/1.1\r\nKeep-Alive: timeout=10, max=20\r\nCookie: contadorCookie=20; Expires=Wed, 21 Oct 2015 07:28:00 GMT; \r\nConnection: Keep-Alive\r\n"};

	*Como se trata el caso de acceso ilegal a directorios superiores de la jerarquia de directorios del sistema. Encontrar coincidencia de \..\
	Ademas La Url puede ser tanto un directorio como un archivo (la comprobacion access, no hace distincion solo comprueba permisos, obviamente si el fichero se encuentra en un directorio
	superior no contara con los permisos)

	https://www.tutorialspoint.com/c_standard_library/c_function_strstr.htm

	*Tratamiento de ficheros -> http://www.chuidiang.org/clinux/ficheros/fichero-binario.php -> lectura y escritura de ficheros
				    Especialmente de ayuda para la lectura y envio del fichero en paquetes de 8kib( el tamno del BUFSIZE) la funcion
				    https://www.tutorialspoint.com/c_standard_library/c_function_fread.htm

				    size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)

				    donde
                                          size: el tamano de bytes que vamos a leer ( en nuestro caso el tamano sera 1)
                                          nmemb: es el numero de elementos del tamano indicado en size que queremos leer del fichero.

				    El avance del puntero del fichero: https://stackoverflow.com/questions/14541940/does-fread-advances-the-pointer-after-it-finishes-reading-the-file



	*Estructura e información sobre las cabeceras de respuesta http -> https://code.tutsplus.com/es/tutorials/http-headers-for-dummies--net-8039
									   https://www.wpdoctor.es/cabeceras-http-mas-comunes/
								           https://es.wikipedia.org/wiki/Anexo:Cabeceras_HTTP
									   https://developer.mozilla.org/es/docs/Web/HTTP/Headers

	*Concatenacion de strings (dos buffers de char) -> https://www.tutorialspoint.com/c_standard_library/c_function_strcat.htm

	*Como obtener fecha actual formateada -> https://poesiabinaria.net/2012/06/obtener-la-fecha-y-hora-formateada-en-c/  (procedimiento)
					    	 http://www.holamundo.es/lenguaje/c/articulos/fecha-hora-c.html (el %formato)
						 https://en.wikipedia.org/wiki/C_date_and_time_functions

	*Cookies -> https://developer.mozilla.org/es/docs/Web/HTTP/Headers/Set-Cookie
		    https://developer.mozilla.org/es/docs/Web/HTTP/Headers/Cookie
		    https://developer.mozilla.org/es/docs/Web/HTTP/Cookies



	*Tamano del fichero solicitado-> podemos aprovechar la estructura stat ,(urlStat), para determinar el tamano del fichero
					 https://linux.die.net/man/2/stat
					 o usando los punteros de los ficheros https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c


	*Cabecera de Persistencia con keepAlive -> https://www.googleessimple.com/wiki/pagespeed/habilitar-keep-alive/
				       		   http://systemadmin.es/2011/08/conexiones-con-keepalive-en-http1-0
				       		   https://es.ryte.com/wiki/Keep_Alive

		KeepAlive: Con esta propiedad, se pueden introducir los valores de encendido y apagado. Para HTTP 1.1, es el valor por defecto.

	   	MaxKeepAliveRequests: Esta propiedad define el máximo de solicitudes posibles por conexión. Un valor entre 50 y 100 suele ser suficiente.

	    	KeepAliveTimeout: Si el servidor no recibe ninguna petición, está inactivo y mantiene la conexión hasta que se cancele.
		La propiedad timeout limita el tiempo que el servidor debe esperar para una nueva petición. Aproximadamente 10 segundos se consideran ideales


		Las cabeceras serian las siguientes:

			Connection: Keep-Alive
			Keep-Alive: timeout=10, max=20



	*Escribir en un socket -> ssize_t write(int fd, const void *buf, size_t count);
				  https://es.wikibooks.org/wiki/Programaci%C3%B3n_en_C/Sockets
				  http://man7.org/linux/man-pages/man2/write.2.html -> a diferencia del read, un caso de exito puede que envie menos datos de los pensados


	*Codigos de Errores -> https://es.wikipedia.org/wiki/Anexo:C%C3%B3digos_de_estado_HTTP

				403 Forbidden
    				La solicitud fue legal, pero el servidor rehúsa responderla dado que el cliente no tiene los privilegios para hacerla.

				404 Not Found
    				Recurso no encontrado. Se utiliza cuando el servidor web no encuentra la página o recurso solicitado.

				405 Method Not Allowed
    				Una petición fue hecha a una URI utilizando un método de solicitud no soportado por dicha URI; por ejemplo, cuando se utiliza GET en un formulario que requiere que los 				datos sean presentados vía POST, o utilizando PUT en un recurso de solo lectura.

				429 Too Many Requests
    				Hay muchas conexiones desde esta dirección de internet.


	*Editor online paginas hmtl ->  https://html5-editor.net/  Permite apartir de un editor de texto generar su correspondiente codigo html


	*Para facilitar el proceso de lectura de ficheros, los errores de lectura, etc... se incluiran explicitamente los ficheros html como buffers. Aunque "chapucera" es una solucion sencilla

	 char buffer404[] =    "HTTP/1.1 404 Not Found\r\n
				Server: Apache\r\n
				Conection: close\r\n
				Content-Length: 362\r\n
				Content-Type: text/html\r\n
				\r\n
				<p>&nbsp;</p>
				<p>&nbsp;</p>
				<p>&nbsp;</p>
				<p>&nbsp;</p>
				<p>&nbsp;</p>
				<p>&nbsp;</p>
				<h1 style="text-align: center;"><span style="color: #993300;">404 File Not Found</span>.</h1>
				<h1 style="text-align: center;">That's an <span style="color: #993300;">Error</span>.</h1>
				<p style="text-align: center;"><strong>The requested URL was not found on this server.</strong></p>"




Main:


	*Recordar que en Linux los directorios son como ficheros,
	Funcion stat para comprobar que es un directorio valido, por un lado hay que comprobar que el fichero existe, y después si es un directorio

		https://www.programacion.com.py/escritorio/c/directorios-y-ficheros-en-c-linux
		http://codewiki.wikidot.com/c:system-calls:stat
		http://forum.codecall.net/topic/68935-how-to-test-if-file-or-directory/
		https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file

		On success, zero is returned.  On error, -1 is returned, and errno is set appropriately.

		Tambien proporciona informacion sobre los permisos, pero no son comprobaciones, más facil con otros metodos

		http://forum.codecall.net/topic/68935-how-to-test-if-file-or-directory/

		int isDirectory(const char *ruta) {
		   struct stat statbuf;
		   if (stat(ruta, &statbuf) != 0)
		       return 0;
		   return S_ISDIR(statbuf.st_mode);
		}


	*Funcion access para comprobar los permisos del directorio
		https://www.ibm.com/support/kFechActualledgecenter/en/SSLTBW_2.3.0/com.ibm.zos.v2r3.bpxbd00/rtacc.htm



	*Comprobacion jerarquia superior de directorios ->

		¿un directorio del sistema?==¿principales directorios como /etc, /lib, /bin...? ->
		Si lo es no se tendran los permisos y saltará la excepcion en la comprobacion anterior

		O

		Aunque tambien podriamos realizar la comprobacion solo considerando como directorios principales.

		La funcion jerarquia solo compara el directorio pasado como argumento y lo compara con los directorios de la jerarquia superior, y devuelve true o false en funcion de si
		son iguales o no. /Home no seria valido, pero /Home/Alumno si.

		int jerarquia(char directorio []){ }
		http://fpsalmon.usc.es/genp/doc/cursos/C++/funciones/pasarArrays.html -> paso de parametros de array de char



		//Comprobacion alternativa de la jerarquia superior de directorios

		int jerarquia(char directorio []){

			    char directorios[18][8096] = { {"/"}
			    ,{"/bin"}
			    ,{"/sbin"}
			    ,{"/boot"}
			    ,{"/dev"}
			    ,{"/etc"}
			    ,{"/home"}
			    ,{"/lib"}
			    ,{"/media"}
			    ,{"/opt"}
			    ,{"/proc"}
			    ,{"/root"}
			    ,{"/srv"}
			    ,{"/sys"}
			    ,{"/tmp"}
			    ,{"/usr"}
			    ,{"/var"}
			    ,{"/../"} };

			   //char *ret;

			    for(int i = 0; i < 18;++i){

				//ret = strstr(directorios[i],directorio);
				//if (ret!=NULL){

				 if (strcmp(directorios[i],directorio)==0){
				    return 0;
				}
			    }
			    return 1;
		}



Extra:

Como calcular cuanto tiempo tarda en ejecutarse una porción de código


	    clock_t t; 
	    t = clock(); 
	    .....
	    codigo a evaluar
	    .....
	    t = clock() - t; 
	    double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds 
	  
	    printf("ªfun() took %f seconds to execute \n", time_taken); 






*/

