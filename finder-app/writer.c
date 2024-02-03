#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char* argv[])
{
    openlog("WRITER: ", 0, LOG_USER);

    if(argc <= 2) {
        syslog(LOG_ERR, "need two arg");
        closelog();
        return 1; 
    }

    int fd = open(argv[1], O_CREAT | O_WRONLY, S_IRWXU | S_IRUSR | S_IROTH);
    int arg1len = strlen(argv[2]);

    if( arg1len == write(fd, argv[2], arg1len))
    {
        syslog(LOG_DEBUG, "Writing %s to %d", argv[1], arg1len);
    }
    else
    {
        syslog(LOG_ERR, "something went wrong");
        closelog();
        return 1;
    }

    closelog();

    return 0;
}