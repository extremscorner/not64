/**
 * Wii64 - FuncTree.c
 * Copyright (C) 2009 Mike Slegeir
 * 
 * Handles a BST of functions ordered by their address
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/

#include <stdlib.h>
#include "../r4300.h"
#include "Recompile.h"

static inline PowerPC_func_node** _find(PowerPC_func_node** node, unsigned short addr){
	while(*node){
		if(addr < (*node)->function->start_addr)
			node = &(*node)->left;
		else if(addr >= (*node)->function->end_addr &&
				(*node)->function->end_addr != 0)
			node = &(*node)->right;
		else
			break;
	}
	return node;
}

PowerPC_func* find_func(PowerPC_func_node** root, unsigned short addr){
	//start_section(FUNCS_SECTION);
	PowerPC_func_node* node = *_find(root, addr);
	//end_section(FUNCS_SECTION);
	return node ? node->function : NULL;
}

void insert_func(PowerPC_func_node** root, PowerPC_func* func){
	PowerPC_func_node** node = _find(root, func->start_addr);
	*node = malloc(sizeof(PowerPC_func_node));
	(*node)->function = func;
	(*node)->left = (*node)->right = NULL;
}

void remove_func(PowerPC_func_node** root, PowerPC_func* func){
	PowerPC_func_node** node = _find(root, func->start_addr);
	PowerPC_func_node* old = *node;
	if(!(*node)->left)
		*node = (*node)->right;
	else if(!(*node)->right)
		*node = (*node)->left;
	else {
		// The node has two children, find the node's predecessor and swap
		PowerPC_func_node** pre;
		for(pre = &(*node)->left; (*pre)->right; pre = &(*pre)->right);
		(*node)->function = (*pre)->function;
		old = *pre;
		*pre = (*pre)->left;
	}

	free(old);
}

