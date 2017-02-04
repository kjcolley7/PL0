//
//  tee.h
//  PL/0
//
//  Created by Kevin Colley on 9/9/15.
//  Copyright (c) 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_TEE_H
#define PL0_TEE_H

#include <stdio.h>

/*! Returns a new file object that writes output to both the file and stdout/stderr
 @param fp File object that references an actual file on disk (will be closed when
           the returned file object is closed)
 @param stdfp Standard file stream such as stdout or stderr that won't be closed
 @return New file object to used instead of @p fp or @p stdfp. Closing this will
         also close @p fp
 */
FILE* ftee(FILE* fp, FILE* stdfp);


#endif /* PL0_TEE_H */
