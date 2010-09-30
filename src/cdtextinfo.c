/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  cdtextinfo, 2008-07-25 by fraxinas <andreas.frisch@multimedia-labs.de>
*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cddb/cddb.h>
#include <cdio/cdio.h>
#include <cdio/cdtext.h>
#include "cddb.h"
#include "getopt.h"

struct opts_s
{
    bool       album;
    bool       listing;
    bool       cddb;
    bool       cdtext;
    bool       xml;
} options;

/* Configuration option codes */
enum {
  OP_HANDLED = 0,

  OP_CDDB_SERVER,
  OP_CDDB_CACHE,
  OP_CDDB_EMAIL,
  OP_CDDB_NOCACHE,
  OP_CDDB_TIMEOUT,

  OP_USAGE,

  /* These are the remaining configuration options */
  OP_VERSION,

};

static bool parse_options (int argc, char *argv[])
{
    int opt; /* used for argument parsing */

    const char* helpText =
       "Usage: %s [OPTION...]\n"
       "  -a, --album                     Print audio CD album info\n"
       "  -l, --listing                   Print title info for all tracks\n"
       "  -T, --cdtext                    Use CD-Text\n"
       "  -D, --cddb                      Use CDDB\n"
       "  -x, --xml                       XML output\n"
       "\n"
       "CDDB options:\n"
       "  -P, --cddb-port=INT             CDDB port number to use (default 8880)\n"
       "  --cddb-server=STRING            CDDB server to contact for information\n"
       "                                  (default: freedb.freedb.org)\n"
       "  --cddb-cache=STRING             Location of CDDB cache directory\n"
       "                                  (default ~/.cddbclient)\n"
       "  --cddb-email=STRING             Email address to give CDDB server\n"
       "                                  (default me@dreambox)\n"
       "  --no-cddb-cache                 Disable caching of CDDB entries\n"
       "                                  locally (default caches)\n"
       "  --cddb-timeout=INT              CDDB timeout value in seconds\n"
       "                                  (default 10 seconds)\n"
       "\n"
       "Help options:\n"
       "  -?, --help                      Show this help message\n"
       "  --usage                         Display brief usage message\n"
       "\n"
       "Example: %s -xalDT\n"
        "  Will query album info and track listing from both CD-Text and CDDB and display as XML\n";

    const char* usageText =
       "Usage: %s [-a|--album] [-l|--listing]\n"
       "        [-T|--cdtext] [-D|--cddb] [-x|--xml]\n"
       "        [-P|--cddb-port INT] [--cddb-server=STRING]\n"
        "        [--cddb-cache=STRING] [--cddb-email=STRING]\n"
        "        [--no-cddb-cache] [--cddb-timeout=INT]\n"
       "        [-?|--help] [--usage]\n";

    const char* optionsString = "alTDP:x?";
    struct option optionsTable[] = {
       {"album", no_argument, NULL, 'a' },
       {"listing", no_argument, NULL, 'l' },
       {"cdtext", no_argument, NULL, 'T' },
       {"cddb", no_argument, NULL, 'D' },
       {"cddb-port", required_argument, NULL, 'P' },
       {"cddb-server", required_argument, NULL, OP_CDDB_SERVER },
       {"cddb-cache", required_argument, NULL, OP_CDDB_CACHE },
       {"cddb-email", required_argument, NULL, OP_CDDB_EMAIL },
       {"no-cddb-cache", no_argument, NULL, OP_CDDB_NOCACHE },
       {"cddb-timeout", required_argument, NULL, OP_CDDB_TIMEOUT },
       {"help", no_argument, NULL, '?' },
       {"usage", no_argument, NULL, OP_USAGE },
       { NULL, 0, NULL, 0 }
    };

    while ((opt = getopt_long(argc, argv, optionsString, optionsTable, NULL)) >= 0) {
       switch (opt) {
       case 'a': options.album = true; break;
       case 'l': options.listing = true; break;
       case 'T': options.cdtext = true; break;
       case 'D': options.cddb = true; break;
       case 'x': options.xml = true; break;
       case 'P': cddb_opts.port = atoi(optarg); break;
       case OP_CDDB_SERVER: cddb_opts.server = strdup(optarg); break;
       case OP_CDDB_CACHE: cddb_opts.cachedir = strdup(optarg); break;
       case OP_CDDB_EMAIL: cddb_opts.email = strdup(optarg); break;
       case OP_CDDB_NOCACHE: cddb_opts.disable_cache = 1; break;
       case OP_CDDB_TIMEOUT: cddb_opts.timeout = atoi(optarg); break;
       case '?':
           fprintf(stdout, helpText, argv[0], argv[0]);
           exit(0);
           break;
       case OP_USAGE:
           fprintf(stderr, usageText, argv[0]);
           exit(1);
           break;
       case 0:
           break;
       }
    }
    return true;
}

static void cddb_errmsg(const char *msg)
{
    if ( options.xml )
       printf("<error msg=\"%s\" />\n", msg);
    else
       printf("%s\n", msg);
}

int print_cddb_info(CdIo_t *p_cdio, track_t i_tracks, track_t i_first_track)
{
    int i_cddb_matches = 0, i_cddb_currentmatch = 1;

    cddb_conn_t *p_conn = NULL;
    cddb_disc_t *p_cddb_disc = NULL;

    if ( init_cddb(p_cdio, &p_conn, &p_cddb_disc, cddb_errmsg, i_first_track, i_tracks, &i_cddb_matches) )
    {
       while ( i_cddb_currentmatch <= i_cddb_matches )
       {
           cddb_read(p_conn, p_cddb_disc);

           if ( options.xml )
               printf("\t<query source=\"CDDB\" match=\"%i\" num_matches=\"%i\">\n",i_cddb_currentmatch,i_cddb_matches);

           if (options.album)
           {
               if ( options.xml )
                   printf("\t\t<albuminfo>\n");

               if ( cddb_disc_get_title(p_cddb_disc) )
               {
                   if ( options.xml )
                       printf("\t\t\t\t<title>%s</title>\n",cddb_disc_get_title(p_cddb_disc));
                   else
                       printf("TITLE:%s\n",cddb_disc_get_title(p_cddb_disc));
               }
               if ( cddb_disc_get_artist(p_cddb_disc) )
               {
                   if ( options.xml )
                       printf("\t\t\t\t<artist>%s</artist>\n",cddb_disc_get_artist(p_cddb_disc));
                   else
                       printf("ARTIST:%s\n",cddb_disc_get_artist(p_cddb_disc));
               }
               if ( cddb_disc_get_genre(p_cddb_disc) )
               {
                   if ( options.xml )
                       printf("\t\t\t\t<genre>%s</genre>\n",cddb_disc_get_genre(p_cddb_disc));
                   else
                       printf("GENRE:%s\n",cddb_disc_get_genre(p_cddb_disc));
               }
               if ( cddb_disc_get_category_str(p_cddb_disc) )
               {
                   if ( options.xml )
                       printf("\t\t\t\t<category>%s</category>\n",cddb_disc_get_category_str(p_cddb_disc));
                   else
                       printf("CATEGORY:%s\n",cddb_disc_get_category_str(p_cddb_disc));
               }
               if ( cddb_disc_get_year(p_cddb_disc) )
               {
                   if ( options.xml )
                       printf("\t\t\t\t<year>%i</year>\n",cddb_disc_get_year(p_cddb_disc));
                   else
                       printf("YEAR:%i\n",cddb_disc_get_year(p_cddb_disc));
               }
               if ( cddb_disc_get_ext_data(p_cddb_disc) )
               {
                   if ( options.xml )
                       printf("\t\t\t\t<extra_data>%s</extra_data>\n",cddb_disc_get_ext_data(p_cddb_disc));
                   else
                       printf("EXTDATA:%s\n",cddb_disc_get_ext_data(p_cddb_disc));
               }
               if ( options.xml )
                   printf("\t\t</albuminfo>\n");
           }
           if (options.listing)
           {
               if ( options.xml )
                   printf("\t\t<tracklisting>\n");

               cddb_track_t *p_cddb_track = cddb_disc_get_track_first(p_cddb_disc);
               while ( p_cddb_track != NULL )
               {
                   if ( options.xml )
                   {
                       printf("\t\t\t\t<track number=\"%i\">\n",cddb_track_get_number(p_cddb_track));
                       if (cddb_track_get_length(p_cddb_track))
                           printf("\t\t\t\t\t\t<length>%i</length>\n",cddb_track_get_length(p_cddb_track));
                       if (cddb_track_get_title(p_cddb_track))
                           printf("\t\t\t\t\t\t<title>%s</title>\n",cddb_track_get_title(p_cddb_track));
                       if (cddb_track_get_artist(p_cddb_track))
                           printf("\t\t\t\t\t\t<artist>%s</artist>\n",cddb_track_get_artist(p_cddb_track));
                       if (cddb_track_get_ext_data(p_cddb_track))
                           printf("\t\t\t\t\t\t<extra_data>%s</extra_data>\n",cddb_track_get_ext_data(p_cddb_track));
                       printf("\t\t\t\t</track>\n");
                   }
                   else
                       printf("%i:%s\n",cddb_track_get_number(p_cddb_track),cddb_track_get_title(p_cddb_track));

                   p_cddb_track = cddb_disc_get_track_next(p_cddb_disc);
               }

               if ( options.xml )
                   printf("\t\t</tracklisting>\n");
           }
           if ( options.xml )
               printf("\t</query>\n");

           cddb_query_next(p_conn, p_cddb_disc);
           i_cddb_currentmatch++;
       }
       if ( i_cddb_currentmatch < 1 && options.xml )
           printf("\t<query source=\"CDDB\" num_matches=\"%i\" />\n",i_cddb_matches);
    }
    cddb_disc_destroy(p_cddb_disc);
    cddb_destroy(p_conn);
    libcddb_shutdown();
    return 0;
}

int print_cdtext_info(CdIo_t *p_cdio, track_t i_tracks, track_t i_track)
{
    const cdtext_t *cdtext = cdio_get_cdtext(p_cdio, 0);

    if ( cdtext == NULL || cdtext && !cdtext->field[9] )
    {
       if ( options.xml )
           printf("\t<query source=\"CD-TEXT\" num_matches=\"0\" />\n");
       return -1;
    }

    if ( options.xml )
       printf("\t<query source=\"CD-TEXT\" match=\"1\" num_matches=\"1\">\n");

    if ( options.album )
    {
       if ( options.xml )
           printf("\t\t<albuminfo>\n");
       cdtext_field_t i;
       for (i=0; i < MAX_CDTEXT_FIELDS; i++)
       {
           if (cdtext->field[i])
           {
               if ( options.xml )
                   printf("\t\t\t\t<%s>%s</%s>\n",cdtext_field2str(i),cdtext->field[i],cdtext_field2str(i));
               else
                   printf("%s:%s\n", cdtext_field2str(i), cdtext->field[i]);
           }
       }
       if ( options.xml )
           printf("\t\t</albuminfo>\n");
    }

    if ( options.listing )
    {
       if ( options.xml )
           printf("\t\t<tracklisting>\n");

       for ( ; i_track < i_tracks; i_track++ )
       {
           const cdtext_t *cdtext = cdio_get_cdtext(p_cdio, i_track);
           if (!cdtext)
               continue;

           if ( options.xml )
           {
               printf("\t\t\t\t<track number=\"%i\">\n",i_track);

               cdtext_field_t i;
               for (i=0; i < MAX_CDTEXT_FIELDS; i++)
               {
                   if (cdtext->field[i])
                   {
                       if ( options.xml )
                           printf("\t\t\t\t\t\t<%s>%s</%s>\n",cdtext_field2str(i),cdtext->field[i],cdtext_field2str(i));
                   }
               }
               printf("\t\t\t\t</track>\n");
           }

           else if (cdtext->field[9])
               printf("%d:%s\n", i_track, cdtext->field[9]);
       }

       if ( options.xml )
           printf("\t\t</tracklisting>\n");
    }

    if ( options.xml )
       printf("\t</query>\n");

    return 0;
}

int main(int argc, char *argv[])
{
    cddb_opts.port     = 8880;
    cddb_opts.server   = NULL;
    cddb_opts.timeout  = 10;
    cddb_opts.disable_cache = false;
    cddb_opts.cachedir = NULL;
    cddb_opts.email = strdup("me@dreambox");

    parse_options(argc, argv);

    track_t i_track;
    track_t i_tracks;
    CdIo_t *p_cdio     = cdio_open (NULL, DRIVER_DEVICE);
    i_tracks           = cdio_get_num_tracks(p_cdio);

    if ( options.xml )
    {
       printf("<?xml version=\"1.0\" ?>\n");
       printf("<cdinfo>\n");
    }

    if (NULL == p_cdio)
       return -1;

    i_track            = cdio_get_first_track_num(p_cdio);
    if ( options.cdtext )
       print_cdtext_info(p_cdio, i_tracks, i_track);

    i_track            = cdio_get_first_track_num(p_cdio);
    if ( options.cddb )
       print_cddb_info(p_cdio, i_tracks, i_track);

    cdio_destroy(p_cdio);

    if ( options.xml )
       printf("</cdinfo>\n");

    return 0;
}
