/*
 * File         : circularBuffer.c
 * Description  : Circular Buffer source
 *
 */
#include "circularBuffer.h"

// Buffer handle internal structure
typedef struct {
    unsigned int   inIndex;
    unsigned int   outIndex;
    unsigned char  full;
    unsigned char  empty;
    unsigned int   size;
    char* bufferData;
} Buffer_t;


BufferHandle_t circularBufferInit(unsigned int size) {
    Buffer_t* newBuffer = NULL;
    // Allocate memory for main structure
#ifdef __KERNEL__
    newBuffer =  kmalloc(sizeof(Buffer_t), GFP_KERNEL);
#else
    newBuffer = (Buffer_t*) malloc(sizeof(Buffer_t));
#endif
    if(newBuffer != NULL) {
        // Allocate memory for internal memory
#ifdef __KERNEL__
        newBuffer->bufferData = (char*) kmalloc(sizeof(char) * size, GFP_KERNEL);
#else
        newBuffer->bufferData = (char*) malloc(sizeof(char) * size);
#endif
        if(newBuffer->bufferData != NULL) {
            // Initialize internal variables
            newBuffer->inIndex = 0;
            newBuffer->outIndex = 0;
            newBuffer->full = 0;
            newBuffer->empty = 1;
            newBuffer->size = size;
        } else {
#ifdef __KERNEL__
            kfree(newBuffer);
#else
            free(newBuffer);
#endif
            newBuffer = NULL;
        }
    }
    return (BufferHandle_t)newBuffer;
}


int circularBufferDelete(BufferHandle_t handle) {
    Buffer_t* buffer = (Buffer_t*) handle;
    int retval = BUFFER_OK;
    // Check buffer structure
    if(buffer == NULL) {
        retval = BUFFER_ERROR;
    } else {
        // Check internal data
        if(buffer->bufferData == NULL) {
            retval = BUFFER_ERROR;
        } else {
#ifdef __KERNEL__
            kfree(buffer->bufferData);
            kfree(buffer);
#else
            free(buffer->bufferData);
            free(buffer);
#endif
        }
    }
    return retval;
}


int circularBufferResize(BufferHandle_t handle, unsigned int newSize) {
    int i = 0;
    int buf_retcode = 0;
    Buffer_t* buffer = (Buffer_t*) handle;
    char *newBufferData;
    unsigned int oldOutIndex;
    if(newSize <= buffer->size)
        return BUFFER_ERROR;

    // print some stuff for debugging
    printk(KERN_WARNING "===circularBuffer_resize: **parameters values before resize**\n");
    printk(KERN_WARNING "===circularBuffer_resize: inIndex=%u\n", buffer->inIndex);
    printk(KERN_WARNING "===circularBuffer_resize: outIndex=%u\n", buffer->outIndex);
    printk(KERN_WARNING "===circularBuffer_resize: full=%u\n", buffer->full);
    printk(KERN_WARNING "===circularBuffer_resize: empty=%u\n", buffer->empty);
    printk(KERN_WARNING "===circularBuffer_resize: size=%u\n", buffer->size);
    printk(KERN_WARNING "===circularBuffer_resize: data=%s\n", buffer->bufferData);

    oldOutIndex = buffer->outIndex;

    // new buffer
    newBufferData = (char*) kmalloc(sizeof(char) * newSize, GFP_KERNEL);
    for(i=0; i<newSize; i++){
        newBufferData[i] = '\0';
    }

    i = 0;
    // fill new buffer with data from circular buffer
    while(i<newSize && !buf_retcode){
        buf_retcode = circularBufferOut(handle, &newBufferData[i]);
        printk(KERN_WARNING "===circularBuffer_resize: circularBufferOut=%i\n", buf_retcode);
        i++;
    }

    // free previous buffer
    kfree(buffer->bufferData);

    // modify buffer params to reflect changes
    buffer->size = newSize;
    buffer->bufferData = newBufferData;
    buffer->full = 0;
    buffer->empty = 0;
    buffer->outIndex = oldOutIndex;

    // reprint params
    printk(KERN_WARNING "===circularBuffer_resize: **parameters values after resize**\n");
    printk(KERN_WARNING "===circularBuffer_resize: inIndex=%u\n", buffer->inIndex);
    printk(KERN_WARNING "===circularBuffer_resize: outIndex=%u\n", buffer->outIndex);
    printk(KERN_WARNING "===circularBuffer_resize: full=%u\n", buffer->full);
    printk(KERN_WARNING "===circularBuffer_resize: empty=%u\n", buffer->empty);
    printk(KERN_WARNING "===circularBuffer_resize: size=%u\n", buffer->size);
    printk(KERN_WARNING "===circularBuffer_resize: data=%s\n", buffer->bufferData);

    return BUFFER_OK;
}


int circularBufferIn(BufferHandle_t handle, char data) {
    Buffer_t* buffer = (Buffer_t*) handle;
    if(buffer->full) {
        return BUFFER_FULL;
    }
    buffer->empty = 0;
    buffer->bufferData[buffer->inIndex] = data;
    buffer->inIndex = (buffer->inIndex + 1) % buffer->size;
    if (buffer->inIndex == buffer->outIndex) {
        buffer->full = 1;
    }
    return BUFFER_OK;
}


int circularBufferOut(BufferHandle_t handle, char *data) {
    Buffer_t* buffer = (Buffer_t*) handle;
    if (buffer->empty) {
        return BUFFER_EMPTY;
    }
    buffer->full = 0;
    *data = buffer->bufferData[buffer->outIndex];
    buffer->outIndex = (buffer->outIndex + 1) % buffer->size;
    if (buffer->outIndex == buffer->inIndex) {
        buffer->empty = 1;
    }
    return BUFFER_OK;
}


unsigned int circularBufferDataCount(BufferHandle_t handle) {
    Buffer_t* buffer = (Buffer_t*) handle;
    int retval = 0;
    if(buffer->inIndex > buffer->outIndex) {
        retval = buffer->inIndex - buffer->outIndex;
    } else if(buffer->inIndex < buffer->outIndex) {
        retval = buffer->size + buffer->inIndex - buffer->outIndex;
    } else if(buffer->full) {
        retval = buffer->size;
    }
    return retval;
}


unsigned int circularBufferDataSize(BufferHandle_t handle) {
   Buffer_t* buffer = (Buffer_t*) handle;
   return buffer->size;
}


unsigned int circularBufferDataRemaining(BufferHandle_t handle) {
   Buffer_t* buffer = (Buffer_t*) handle;
   return buffer->size - circularBufferDataCount(handle);
}
