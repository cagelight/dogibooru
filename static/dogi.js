class DOGI {

  static JSONPost(url, json, func, doerr = true) {
    let request = new XMLHttpRequest()
    request.open('POST', url, true)
    request.setRequestHeader("Content-Type", "application/json")
    request.onload = function() {
      if (request.status == 200 || request.status == 304) {
        let data = {}
        try {
          data = JSON.parse(request.responseText);
        } catch (ex) {
          if (doerr) DOGI.ErrorPopup(ex + ":\n" + request.responseText)
          func({}, false)
          return
        }
        if (data) {
          func(data, true)
        } else {
          func({}, false)
        }
      } else {
        if (doerr) DOGI.ErrorPopup(request.responseText)
        func({}, false)
      }
    };
    request.send(JSON.stringify(json))
  }

  static JSONGet(url, func, doerr = true) {
  	let request = new XMLHttpRequest();
  	request.open('GET', url, true);
  	request.onload = function() {
  		if (request.status == 200 || request.status == 304) {
        let data = {}
        try {
          data = JSON.parse(request.responseText);
        } catch (ex) {
          if (doerr) DOGI.ErrorPopup(ex + ":\n" + request.responseText)
          func({}, false)
          return
        }
        if (data) {
          func(data, true)
        } else {
          func({}, false)
        }
  		} else {
        if (doerr) DOGI.ErrorPopup(request.responseText)
        func({}, false)
      }
  	};
  	request.send();
  }

  static ClearElement(e) {
    while (e.hasChildNodes()) {
      e.removeChild(e.lastChild);
    }
  }

  static ChildIterate(e, func) {
    Array.from(e.childNodes).forEach((child)=>{func(child)})
  }

  static ErrorPopup(text) {
    if (this.err != null) {
      document.body.removeChild(this.err)
    }
    this.err = document.createElement('iframe')
    this.err.src = "about:blank"
    this.err.className = "dogi_error"
    document.body.appendChild(this.err)
    this.err.contentDocument.open()
    this.err.contentDocument.write(text)
    this.err.contentDocument.close()
    this.err.contentDocument.addEventListener("click", (event) => {
      document.body.removeChild(this.err)
      this.err = null
    })
    this.err.contentDocument.body.style.padding = "10px"
    this.err.contentDocument.body.style.backgroundColor = "#F00"
    this.err.contentDocument.body.style.color = "#000"
  }

  static ReadableFilesize(num_bytes) {
    if (num_bytes < 2048) return num_bytes + " B"
    if (num_bytes < 2097152) return (num_bytes / 1024).toFixed(1) + " KiB"
    if (num_bytes < 2147483648) return (num_bytes / 2097152).toFixed(1) + " MiB"
    return num_bytes + (num_bytes / 2147483648).toFixed(1) + " GiB"
  }

  static WrapSpan(text, classn = null) {
    let span = document.createElement('span')
    if (classn) span.className = classn
    span.appendChild(document.createTextNode(text))
    return span
  }

}

String.prototype.hashCode = function(seed = 42){
  var remainder, bytes, h1, h1b, c1, c1b, c2, c2b, k1, i;

	remainder = this.length & 3; // this.length % 4
	bytes = this.length - remainder;
	h1 = seed;
	c1 = 0xcc9e2d51;
	c2 = 0x1b873593;
	i = 0;

	while (i < bytes) {
	  	k1 =
	  	  ((this.charCodeAt(i) & 0xff)) |
	  	  ((this.charCodeAt(++i) & 0xff) << 8) |
	  	  ((this.charCodeAt(++i) & 0xff) << 16) |
	  	  ((this.charCodeAt(++i) & 0xff) << 24);
		++i;

		k1 = ((((k1 & 0xffff) * c1) + ((((k1 >>> 16) * c1) & 0xffff) << 16))) & 0xffffffff;
		k1 = (k1 << 15) | (k1 >>> 17);
		k1 = ((((k1 & 0xffff) * c2) + ((((k1 >>> 16) * c2) & 0xffff) << 16))) & 0xffffffff;

		h1 ^= k1;
        h1 = (h1 << 13) | (h1 >>> 19);
		h1b = ((((h1 & 0xffff) * 5) + ((((h1 >>> 16) * 5) & 0xffff) << 16))) & 0xffffffff;
		h1 = (((h1b & 0xffff) + 0x6b64) + ((((h1b >>> 16) + 0xe654) & 0xffff) << 16));
	}

	k1 = 0;

	switch (remainder) {
		case 3: k1 ^= (this.charCodeAt(i + 2) & 0xff) << 16;
		case 2: k1 ^= (this.charCodeAt(i + 1) & 0xff) << 8;
		case 1: k1 ^= (this.charCodeAt(i) & 0xff);

		k1 = (((k1 & 0xffff) * c1) + ((((k1 >>> 16) * c1) & 0xffff) << 16)) & 0xffffffff;
		k1 = (k1 << 15) | (k1 >>> 17);
		k1 = (((k1 & 0xffff) * c2) + ((((k1 >>> 16) * c2) & 0xffff) << 16)) & 0xffffffff;
		h1 ^= k1;
	}

	h1 ^= this.length;

	h1 ^= h1 >>> 16;
	h1 = (((h1 & 0xffff) * 0x85ebca6b) + ((((h1 >>> 16) * 0x85ebca6b) & 0xffff) << 16)) & 0xffffffff;
	h1 ^= h1 >>> 13;
	h1 = ((((h1 & 0xffff) * 0xc2b2ae35) + ((((h1 >>> 16) * 0xc2b2ae35) & 0xffff) << 16))) & 0xffffffff;
	h1 ^= h1 >>> 16;

	return h1 >>> 0;
}
