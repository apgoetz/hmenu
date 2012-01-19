#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <fcntl.h>
#include <ctype.h>
#include <EXTERN.h>
#include <perl.h>

#define BUFFSIZE 1024
#define DEFAULT_DMENU "/usr/bin/dmenu"
int get_item_count(sqlite3 *dbhandler);
void inc_item(char* item, int amount, sqlite3 *dbhandler);
int CallFunc(char* funcname, char* funcparam);

static PerlInterpreter *my_perl;
int main(int argc, char ** argv, char **env)
{
	char buffer[BUFFSIZE];
	int childin[2];
	int childout[2];
	int retval;
	char* end;
	int childstatus;
	char newline = '\n';
	char *tmpchar;
	int perlfd;
	char *dmenu_executable;
	char *param;
	char *stem;
	char **newargs;
	int i;
	char *perlscript;
	sqlite3 *dbhandler;
	char * create_table_query = 
		"CREATE TABLE IF NOT EXISTS hmenu"
		"(item TEXT, count INTEGER, priority INTEGER);";
	char * get_items_query = "SELECT item from hmenu "
		"ORDER BY count DESC, priority DESC;";
	const char * curitem;
	sqlite3_stmt *statement;
	int dbfile;
	pipe(childin);
	pipe(childout);
	char * dbname = NULL;
	
	dbname = getenv("HMENU_DB");

	/* the dbname was not passed on the command line. We still
	 * need to copy all of the parameters, but we don't need to
	 * remove the filename, as it wasn't passed as a parameter. */
	/* create a copy of the parameters that is null terminted */
	newargs = (char**)malloc(sizeof(char*)*(argc + 1));
	for(i = 1; i < argc; i++) {
		newargs[i] = (char*)malloc(sizeof(char)*strlen(argv[i] + 1));
		strcpy(newargs[i], argv[i]);
	}
	newargs[argc] = 0;
	
	dmenu_executable = getenv("HMENU_CHILDMENU");

	if(!dmenu_executable)
		dmenu_executable = DEFAULT_DMENU;
	
	tmpchar = strrchr(dmenu_executable, '/');
	
	if(tmpchar != NULL)
		newargs[0] = tmpchar + 1;
	else
		newargs[0] = dmenu_executable;

	
	
	/* if the file does not exist */
	if(access(dbname, F_OK) == -1) {
		sqlite3_open(dbname, &dbhandler);
		sqlite3_prepare(dbhandler, create_table_query, 
				strlen(create_table_query),
				&statement, NULL);
		sqlite3_step(statement);
		sqlite3_finalize(statement);
	} else {
		sqlite3_open(dbname, &dbhandler);
	}
	 	

	

	
	retval = fork();
	/* if we are the child */
	if(!retval) {
		
		dup2(childout[1], 1);
		
		/* if dbname was specified, we act like special
		 * menu. Otherwise use stdin like dmenu */
		if(dbname) {
			close(childin[1]);
			dup2(childin[0], 0);			
		} 
		
		execv(dmenu_executable, newargs);

		/* we are the parent */
	} else {
		free(newargs);
		/* we don't read input */
		close(childin[0]);
		/* dont write output */
		close(childout[1]);
		/* set the output of the child as our input */
		dup2(childout[0],0);

		/* if the dbname was not provided, we act like regular
		 * dmenu*/
		if(dbname == NULL)
		{
			wait(&childstatus);
			retval = fread(buffer, sizeof(char), BUFFSIZE, stdin);
			buffer[retval] = '\0';
			printf("%s\n", buffer);
			return 0;
					
		}
		/* get a list of all of the menu items, sorted by frequency of use */
		sqlite3_prepare(dbhandler, get_items_query, 
				strlen(get_items_query), &statement, NULL);
		retval = sqlite3_step(statement);
		/* print out the list to stdout*/
		while (retval != SQLITE_DONE) {
			curitem = sqlite3_column_text(statement, 0);
			write(childin[1], curitem, strlen(curitem));
			write(childin[1], &newline, 1);
			retval = sqlite3_step(statement);
		} 
		sqlite3_finalize(statement);

	       
		/* done with input */
		close(childin[1]);
		/* wait until dmenu finishes */
		wait(&childstatus);
		retval = fread(buffer, sizeof(char), BUFFSIZE, stdin);
		/* if we actually read something */
		if(retval != 0) {
			
			/* remove ending newline, if it existst */
			tmpchar = NULL;
			tmpchar = strrchr(buffer, '\n');			
			if(tmpchar)
				*tmpchar = '\0';
			end = buffer + strlen(buffer) - 1;
			/* remove trailing whitespace */
			while(end > buffer && isspace(*end)) end--;
			*(end + 1) = 0;
			
			
			tmpchar = (char*)malloc(sizeof(char)*strlen(buffer)+1);
			strcpy(tmpchar, buffer);
			stem = strtok(tmpchar, " ");


			
			perlscript = getenv("HMENU_PERL");
			if(perlscript && 
			   access(perlscript,F_OK) != -1) {
				char *perlargs[] = {"perl", perlscript};
				PERL_SYS_INIT3(&argc,&argv,&env);
				my_perl = perl_alloc();
				perl_construct(my_perl);
				perl_parse(my_perl, NULL, 2, perlargs, NULL);
				PL_exit_flags |= PERL_EXIT_DESTRUCT_END;

				if(strcmp(stem, buffer)) {
					param = buffer + strlen(stem) + 1;
					inc_item(stem, 1, dbhandler);
				} else {
					param = "";
				}
				/* if we could not call the perl
				 * function, see if file exists in
				 * path */
				if(CallFunc(stem, param)) {
					printf("%s\n", buffer);
				}
					
				
				perl_destruct(my_perl);
				perl_free(my_perl);
				PERL_SYS_TERM();
			}
			else
			{
				if(strcmp(stem, buffer)){
					param = buffer + strlen(stem) + 1;
					inc_item(stem, 1, dbhandler);
					
					printf("%s\n", buffer);
				}
				else
				{
					printf("%s\n", buffer);
				}
			}

			
			free(tmpchar);
			
			inc_item(buffer, 2, dbhandler);			
			
		}
		close(childout[0]);
		
		sqlite3_close(dbhandler);
		
	}
}


void inc_item(char* item, int amount, sqlite3 *dbhandler)
{
	char * exists_query = "SELECT count FROM hmenu WHERE item = ?;";
	char * insert_query = "INSERT INTO hmenu(item, count, priority) "
		"VALUES (?, ?, ?);";
	char * update_query = "UPDATE hmenu SET count = ?, priority = ? "
		"WHERE item = ?;";
	sqlite3_stmt *statement;
	int retval;
	int itemcount;
	int maxpriority;
	maxpriority = get_item_count(dbhandler);
	
	sqlite3_prepare(dbhandler, exists_query, 
			strlen(exists_query), &statement, NULL);
	sqlite3_bind_text
		(statement, 1, item, strlen(item), SQLITE_TRANSIENT);
	retval = sqlite3_step(statement);
	
	/* if it isn't in the database yet */
	if(retval == SQLITE_DONE) {
		sqlite3_finalize(statement);
		sqlite3_prepare(dbhandler, insert_query, strlen(insert_query), 
				&statement, NULL);
		/* item */
		sqlite3_bind_text(statement, 1, item, strlen(item), 
				  SQLITE_TRANSIENT);
		/* count */
		sqlite3_bind_int(statement, 2, amount);
		/* priority */
		sqlite3_bind_int(statement, 3, maxpriority + 1);

		sqlite3_step(statement);
		
		
		/* if it is in the table */
	} else {
		itemcount = sqlite3_column_int(statement, 0);
		sqlite3_finalize(statement);
		sqlite3_prepare(dbhandler, update_query,
				strlen(update_query),
				&statement, NULL);
		/* item to change */
		sqlite3_bind_text
			(statement, 3, item, 
			 strlen(item), SQLITE_TRANSIENT);
		/* count to set it to*/
		sqlite3_bind_int(statement, 1, itemcount + amount);
		/* priority to set it to */
		sqlite3_bind_int(statement, 2, maxpriority + 1);
		sqlite3_step(statement);

	}
	sqlite3_finalize(statement);

}

int get_item_count(sqlite3 *dbhandler)
{
	char* query = "SELECT MAX(priority) FROM hmenu;";
	sqlite3_stmt *statement;
	int count;
	int retval;
	sqlite3_prepare(dbhandler, query,
				strlen(query),
				&statement, NULL);
	retval = sqlite3_step(statement);
	
	count = sqlite3_column_int(statement, 0);
	sqlite3_finalize(statement);
	return count;
}
int CallFunc(char* funcname, char* funcparam)
{
	dSP;
	int count;
	int retval;
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);
	XPUSHs(sv_2mortal(newSVpv(funcparam, 0)));
	PUTBACK;
	count = call_pv(funcname, G_EVAL|G_DISCARD);
	SPAGAIN;
	/* Check the eval first */
	/* if it is true, there was an error */
	if (SvTRUE(ERRSV))
	{
		retval = -1;
	}
	else
	{
		retval = 0;
	}
	PUTBACK;
	FREETMPS;
	LEAVE;
	return retval;
	
}
