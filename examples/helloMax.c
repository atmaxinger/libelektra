//
// Created by maximilianirlinger on 26.01.22.
//

#include <kdb.h>

#include <stdio.h>

int main(void)
{
	Key *parentKey = keyNew ("/test", KEY_END);
	KDB *repo = kdbOpen(NULL, parentKey);
	KeySet *conf = ksNew(200, KS_END);

	kdbGet (repo, conf, parentKey);

	Key *k = ksLookupByName (conf, "/test/hello", 0);
	printf("%s\n", keyString (k));

	/*KeySet * config = ksNew (0, KS_END);
	Key * root = keyNew ("user:/test", KEY_END);

	printf ("Open key database\n");
	KDB * handle = kdbOpen (NULL, root);

	printf ("Retrieve key set\n");
	kdbGet (handle, config, root);

	printf ("Number of key-value pairs: %zd\n", ksGetSize (config));

	Key *k = ksLookupByName (config, "hello", 0);
	printf("%s\n", keyString (k));

	printf ("Delete key-value pairs inside memory\n");
	ksDel (config);
	printf ("Close key database\n");
	kdbClose (handle, 0);*/
}