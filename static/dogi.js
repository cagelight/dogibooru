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

String.prototype.hashCode = function(){
  var hash = 5381;
  for (i = 0; i < this.length; i++) {
      char = this.charCodeAt(i);
      hash = ((hash << 5) + hash) + char; /* hash * 33 + c */
  }
  return hash;
}
