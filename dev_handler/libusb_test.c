#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void clean_up (libusb_device_handle *handle, libusb_context *ctx, libusb_device **lst)
{
	libusb_close(handle);
	libusb_free_device_list(lst, 1);
	libusb_exit(ctx);
}

int main ()
{
	int ret;
	uint8_t ix;
	
	libusb_context *ctx;
	libusb_device **lst;
	libusb_device *dev = NULL;
	libusb_device_handle *handle = NULL;
	struct libusb_device_descriptor *desc = (struct libusb_device_descriptor *) malloc (sizeof(struct libusb_device_descriptor) * 1);
	unsigned char *data_buf = (unsigned char *) malloc (sizeof(unsigned char) * 256);
	
	//initialize the libusb context, check for errors and exit + return on failure
	if ((ret = libusb_init(&ctx)) != 0) {
		printf("libusb_init failed on exit code %d\n", ret);
		libusb_exit(ctx);
		return 1;
	}
	
	ret = libusb_get_device_list(ctx, &lst); //get the list of all attached USB devices
	printf("number of devices found: %d\n", ret);
	
	//if we found negative devices, return
	if (ret < 0) {
		return 2;
	}
	
	dev = lst[0]; //just one device
	
	//open a handle on the device
	if ((ret = libusb_open(dev, &handle)) != 0) {
		printf("libusb_open failed on exit code %d\n", ret);
		clean_up(handle, ctx, lst);
		return 3;
	}
	
	//get device descriptor and print out string information
	if ((ret = libusb_get_device_descriptor(dev, desc)) != 0) {
		printf("libusb_get_device_descriptor failed on exit code %d\n", ret);
		clean_up(handle, ctx, lst);
		return 4;
	}
	printf("vendor ID = %d\n", desc->idVendor);
	printf("product ID = %d\n", desc->idProduct);
	
	//note: 0x0409 is the language ID for English: United States
	ix = desc->iManufacturer;
	ret = libusb_get_string_descriptor_ascii(handle, ix, data_buf, 256);
	printf("manufacturer: %s, bytes returned = %d\n", data_buf, ret);
	
	ix = desc->iProduct;
	ret = libusb_get_string_descriptor_ascii(handle, ix, data_buf, 256);
	printf("product: %s, bytes returned = %d\n", data_buf, ret);
	
	ix = desc->iSerialNumber;
	ret = libusb_get_string_descriptor_ascii(handle, ix, data_buf, 256);
	printf("serial number: %s, bytes returned = %d\n", data_buf, ret);
	
	//free malloced stuff
	free(desc);
	free(data_buf);
	
	libusb_close(handle); //close the device handle
	
	libusb_free_device_list(lst, 1); //free the device list while also unreferencing all other detected devices
	
	libusb_exit(ctx); //deinitialize the libusb context
	
	return 0;
}