#include "main.h"

int read_device(const char *devfile)
{
	struct input_event ev;
	size_t i;
	ssize_t s;
	int e = 0;
	bool on = true;
	int fd = open(devfile, O_RDONLY);

	if (fd < 0) {
		std::string err(strerror(errno));
		cerr << "Failed to open device '" << devfile << "': " << err << endl;
		return 1;
	}

	if (ioctl(fd, EVIOCGRAB, (void*)1) == -1) {
		std::string err(strerror(errno));
		cerr << "Failed to grab device: " << err << endl;
	}

	struct uinput_user_dev dev;
	memset(&dev, 0, sizeof(dev));

	if (ioctl(fd, EVIOCGNAME(sizeof(dev.name)), dev.name) == -1) {
		cErr << "Failed to get device name: " << err << endl;
		goto error;
	}

	if (ioctl(fd, EVIOCGID, &dev.id) == -1) {
		cErr << "Failed to get device id: " << err << endl;
		goto error;
	}
	
	cerr << " Device: " << dev.name << endl;
	cerr << "     Id: " << dev.id.version << endl;
	cerr << "BusType: " << dev.id.bustype << endl;
	
	cout.write(dev.name, sizeof(dev.name));
	cout.write((const char*)&dev.id, sizeof(dev.id));
	
	cerr << "Getting input bits." << endl;
	if (ioctl(fd, EVIOCGBIT(0, sizeof(input_bits)), &input_bits) == -1) {
		cErr << "Failed to get input-event bits: " << err << endl;
		goto error;
	}
	cout.write((const char*)input_bits, sizeof(input_bits));

#define TransferBitsFor(REL, rel, REL_MAX) \
	do { \
	if (testbit(input_bits, EV_##REL)) { \
		unsigned char bits##rel[1+REL_MAX/8]; \
		cerr << "Getting " #rel "-bits." << endl; \
		if (ioctl(fd, EVIOCGBIT(EV_##REL, sizeof(bits##rel)), bits##rel) == -1) { \
			cErr << "Failed to get " #rel " bits: " << err << endl; \
			goto error; \
		} \
		cout.write((const char*)&bits##rel, sizeof(bits##rel)); \
	} \
	} while(0)
	
	TransferBitsFor(KEY, key, KEY_MAX);
	TransferBitsFor(ABS, abs, ABS_MAX);
	TransferBitsFor(REL, rel, REL_MAX);
	TransferBitsFor(MSC, msc, MSC_MAX);
	TransferBitsFor(SW, sw, SW_MAX);
	TransferBitsFor(LED, led, LED_MAX);

#define TransferDataFor(KEY, key, KEY_MAX) \
	do { \
	if (testbit(input_bits, EV_##KEY)) { \
		cerr << "Getting " #key "-state." << endl; \
		unsigned char bits##key[1+KEY_MAX/8]; \
		if (ioctl(fd, EVIOCG##KEY(sizeof(bits##key)), bits##key) == -1) { \
			cErr << "Failed to get " #key " state: " << err << endl; \
			goto error; \
		} \
		cout.write((const char*)bits##key, sizeof(bits##key)); \
	} \
	} while(0)
	
	TransferDataFor(KEY, key, KEY_MAX);
	TransferDataFor(LED, led, LED_MAX);
	TransferDataFor(SW, sw, SW_MAX);

	if (testbit(input_bits, EV_ABS)) {
		struct input_absinfo ai;
		cerr << "Getting abs-info." << endl;
		for (i = 0; i < ABS_MAX; ++i) {
			if (ioctl(fd, EVIOCGABS(i), &ai) == -1) {
				cErr << "Failed to get device id: " << err << endl;
				goto error;
			}
			cout.write((const char*)&ai, sizeof(ai));
		}
	}

	cout.flush();
	cerr << "Transferring input events." << endl;
	while (true) {
		s = read(fd, &ev, sizeof(ev));
		if (!s) {
			cerr << "EOF" << endl;
			break;
		}
		else if (s < 0) {
			cErr << "When reading from device: " << err << endl;
			goto error;
		}

		if (ev.type == EV_KEY && ev.code == 161) {
			if (ev.value == 0)
				on = !on;
			if (ioctl(fd, EVIOCGRAB, (void*)(int)on) == -1) {
				cErr << "Grab failed: " << err << endl;
			}
		} else {
			cout.write((const char*)&ev, sizeof(ev));
			cout.flush();
		}
	}

	goto end;
error:
	e = 1;
end:
	ioctl(fd, EVIOCGRAB, (void*)0);
	close(fd);

	return 0;
}