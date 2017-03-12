class persistent {
  constructor(hash) {
    let hashsplit = hash.split(':')
    this.tags = []
    hashsplit[0].split(',').forEach((tag)=>{if(tag)this.tags.push(tag)})
    this.npp = parseInt(hashsplit[1]) || 20;
    this.offs = parseInt(hashsplit[2]) || 0;
  }
  sethash() {
    window.location.hash = this.tags.join(',') + ':' + this.npp + ':' + this.offs
  }
}

var last_pers = new persistent("")
var pers = new persistent("")

function pers_changed() {
  if (pers.tags.length != last_pers.tags.length) return true
  for (let i = 0; i < pers.tags.length; i++) {
    if (pers.tags[i] != last_pers.tags[i]) {
      return true
    }
  }
  return false
}

function setup_page() {
  let filterj = {}
  filterj.tags = pers.tags
  let test = document.getElementById("index_imgview")
  DOGI.ClearElement(test)
  DOGI.JSONPost("/api/filter", filterj, function(result){
    result.forEach((field)=>{
      let entrycont = document.createElement("div")
      entrycont.className = 'index_entry'
      let thumbcont = document.createElement("div")
      thumbcont.className = 'index_thumbcont'
      let viewlink = document.createElement("a")
      viewlink.href = "/view#" + field
      let thumb = document.createElement("img")
      thumb.src = "/t/" + field + "_200.jpg"
      thumbcont.appendChild(viewlink)
      viewlink.appendChild(thumb)
      entrycont.appendChild(thumbcont)
      test.appendChild(entrycont)
    });
  });
}

function update_page() {
  last_pers = pers
  pers = new persistent(window.location.hash.substring(1))
  if (pers_changed()) setup_page()
  pers.sethash()
}

window.onhashchange = function(){
  update_page()
};
document.addEventListener("DOMContentLoaded", function(event) {
  pers = new persistent(window.location.hash.substring(1))
  setup_page()
  pers.sethash()
});
