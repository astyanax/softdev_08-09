#ifndef __VW_LIB_H__
#define __VW_LIB_H__

int VW_createSelect( int argc, char *argv[] );
int VW_createJoin( int argc, char *argv[] );
int VW_delete( int argc, char *argv[] );
int VW_insert( int argc, char *argv[] );
int VW_destroy( int argc, char *argv[] );

int updateViewCat( viewDesc * viewInfo, char valueType );
int updateViewAttrCat( viewAttrDesc * viewAttrInfo );

int getViewRelation( char * viewRelName, viewDesc * view, viewAttrDesc ** viewAttrs );
char ** getViewsOfRelation( char * relName, int * nviews );
int recursive_update( char * viewName, char * relName, char * tempName, int flag, int tempIndex );

#endif   /*  __VW_LIB_H__  */

