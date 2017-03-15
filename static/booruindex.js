class persistent {
  constructor(hash) {
    let hashsplit = hash.split(':')
    this.tags = []
    this.omit = []
    if (hashsplit[0]) hashsplit[0].split(',').forEach((tag)=>{if(tag)this.tags.push(tag)})
    if (hashsplit[1]) hashsplit[1].split(',').forEach((tag)=>{if(tag)this.omit.push(tag)})
    this.npp = parseInt(hashsplit[2]) || 20;
    this.offs = parseInt(hashsplit[3]) || 0;
  }
  genhash() {
    return this.tags.join(',') + ':' + this.omit.join(',') + ':' + this.npp + ':' + this.offs
  }
  sethash() {
    window.location.hash = this.genhash()
  }
  clonefrom(other) {
    this.tags = []
    other.tags.forEach((tag)=>{this.tags.push(tag)})
    this.omit = []
    other.omit.forEach((tag)=>{this.omit.push(tag)})
    this.npp = other.npp
    this.ofs = other.offs
  }
}

var last_pers = new persistent("")
var pers = new persistent("")

function pers_changed() {
  if (pers.tags.length != last_pers.tags.length) return true
  if (pers.omit.length != last_pers.omit.length) return true
  for (let i = 0; i < pers.tags.length; i++) {
    if (pers.tags[i] != last_pers.tags[i]) {
      return true
    }
  }
  for (let i = 0; i < pers.omit.length; i++) {
    if (pers.omit[i] != last_pers.omit[i]) {
      return true
    }
  }
  return false
}

function setup_page() {
  let filterj = {}
  filterj.tags = pers.tags
  filterj.omit = pers.omit
  let index_imgview = document.getElementById("index_imgview")
  let index_leftbar = document.getElementById("index_leftbar")
  DOGI.ClearElement(index_imgview)
  DOGI.ClearElement(index_leftbar)
  DOGI.JSONPost("/api/filter", filterj, function(result){
    result.forEach((field)=>{
      //================
      let entrycont = document.createElement("div")
      entrycont.className = 'index_entry'
      //================
      let thumbcont = document.createElement("div")
      thumbcont.className = 'index_thumbcont'
      //================
      let viewlink = document.createElement("a")
      viewlink.href = "/view#" + field
      //================
      let thumb = document.createElement("img")
      thumb.src = "/t/" + field + "_200.jpg"
      //================
      let imginfo_res = document.createElement('span')
      let imginfo_w = document.createElement('span')
      let imginfo_h = document.createElement('span')
      let imginfo_siz = document.createElement('span')
      imginfo_res.className = 'index_entry_text index_res'
      imginfo_w.className = 'index_entry_text index_res_hw'
      imginfo_h.className = 'index_entry_text index_res_hw'
      imginfo_siz.className = 'index_entry_text index_siz'
      DOGI.JSONPost("/api/img", [field], (info)=>{
        imginfo_w.appendChild(document.createTextNode(info[field].width))
        imginfo_h.appendChild(document.createTextNode(info[field].height))
        imginfo_res.appendChild(imginfo_w)
        imginfo_res.appendChild(document.createTextNode('x'))
        imginfo_res.appendChild(imginfo_h)
        imginfo_siz.appendChild(document.createTextNode(DOGI.ReadableFilesize(info[field].size)))
      })
      //================
      thumbcont.appendChild(viewlink)
      viewlink.appendChild(thumb)
      entrycont.appendChild(thumbcont)
      entrycont.appendChild(imginfo_res)
      entrycont.appendChild(imginfo_siz)
      index_imgview.appendChild(entrycont)
    });
  });
  let tagsrelobj = {}
  tagsrelobj.tags = pers.tags
  tagsrelobj.omit = pers.omit
  DOGI.JSONPost("/api/tagsrel", tagsrelobj, (data)=>{
    data.forEach((tag)=>{
      let tagstuff = document.createElement('div')
      tagstuff.className = 'index_leftbar_tagcont';
      //================
      let tagname = document.createElement('span')
      tagname.appendChild(document.createTextNode(tag.name))
      let tagnamelink = document.createElement('a')
      tagnamelink.className = 'tgrp_grp_' + ' index_leftbar_tagname'
      //========
      taglpers = new persistent('')
      taglpers.clonefrom(pers)
      taglpers.tags = [tag.name]
      taglpers.omit = []
      tagnamelink.href = '#' + taglpers.genhash()
      //========
      tagnamelink.appendChild(tagname)
      tagstuff.appendChild(tagnamelink)
      //================
      let tagcontrols = document.createElement('span')
      //========
      let tagcontroladd = document.createElement('a')
      tcapers = new persistent('')
      tcapers.clonefrom(pers)
      tcapers.tags.push(tag.name)
      tagcontroladd.href = '#' + tcapers.genhash()
      tagcontroladd.className = 'index_leftbar_options';
      tagcontroladd.appendChild(document.createTextNode('➕'))
      tagcontrols.appendChild(tagcontroladd)
      //========
      let tagcontrolrem = document.createElement('a')
      tcrpers = new persistent('')
      tcrpers.clonefrom(pers)
      tcrpers.omit.push(tag.name)
      tagcontrolrem.href = '#' + tcrpers.genhash()
      tagcontrolrem.className = 'index_leftbar_options';
      tagcontrolrem.appendChild(document.createTextNode('➖'))
      tagcontrols.appendChild(tagcontrolrem)
      //========
      tagstuff.appendChild(tagcontrols)
      //================
      let tagcount = document.createElement('span')
      tagcount.appendChild(document.createTextNode('(' + tag.count + ')'))
      tagcount.className = 'index_leftbar_tagcount'
      tagstuff.appendChild(tagcount)
      //================
      index_leftbar.appendChild(tagstuff)
    })
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
