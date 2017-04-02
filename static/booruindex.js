var index_imgview
var index_leftbar
var index_taginput
var index_omitinput

class persistent {
  constructor(hash) {
    let hashsplit = hash.split(':')
    this.tags = []
    this.omit = []
    if (hashsplit[0]) hashsplit[0].split(',').forEach((tag)=>{if(tag)this.tags.push(tag)})
    if (hashsplit[1]) hashsplit[1].split(',').forEach((tag)=>{if(tag)this.omit.push(tag)})
    this.mode = hashsplit[2] || 'filter'
    this.npp = parseInt(hashsplit[3]) || 20
    this.offs = parseInt(hashsplit[4]) || 0
    if (!['filter','lsim','ssim'].includes(this.mode)) this.mode = 'filter'
  }
  genhash() {
    return this.tags.join(',') + ':' + this.omit.join(',') + ':' + this.mode + ':' + this.npp + ':' + this.offs
  }
  sethash() {
    window.location.hash = this.genhash()
  }
  clonefrom(other) {
    this.tags = []
    other.tags.forEach((tag)=>{this.tags.push(tag)})
    this.omit = []
    other.omit.forEach((tag)=>{this.omit.push(tag)})
    this.mode = other.mode
    this.npp = other.npp
    this.ofs = other.offs
  }
  clone() {
    let npers = new persistent("")
    npers.clonefrom(this)
    return npers
  }
}

var last_pers = new persistent("")
var pers = new persistent("")

var pers_change = { // enums in JS lol
  'none' : 0,
  'nppoffs' : 1,
  'mode' : 2,
  'tags' : 3
}

function setup_thumbs(ids, weights) {
  DOGI.ClearElement(index_imgview)
  for (let i = 0; i < ids.length; i++) {
    let field = ids[i]
    let weight
    if (weights) weight = weights[i]
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
    thumb.src = "/t/" + field + "_200.png"
    //================
    let imginfo_cont = document.createElement('span')
    imginfo_cont.className = 'index_entry_textcont'
    let imginfo_res = document.createElement('span')
    let imginfo_weight = document.createElement('span')
    let imginfo_w = document.createElement('span')
    let imginfo_h = document.createElement('span')
    let imginfo_siz = document.createElement('span')
    imginfo_res.className = 'index_entry_text index_res'
    imginfo_weight.className = 'index_entry_text index_weight'
    if (weight) imginfo_weight.appendChild(document.createTextNode(weight))
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
    imginfo_cont.appendChild(imginfo_res)
    imginfo_cont.appendChild(imginfo_weight)
    imginfo_cont.appendChild(imginfo_siz)
    entrycont.appendChild(imginfo_cont)
    index_imgview.appendChild(entrycont)
  }
}

function setup_tagsrel(data) {
  DOGI.ClearElement(index_leftbar)
  let leftbar_tagmap = {}
  data.forEach((tag)=>{
    let tagstuff = document.createElement('div')
    tagstuff.className = 'index_leftbar_tagcont'
    //================
    let tagname = document.createElement('span')
    tagname.appendChild(document.createTextNode(tag.name))
    let tagnamelink = document.createElement('a')
    tagnamelink.className = 'index_leftbar_tagname'
    leftbar_tagmap[tag.name] = tagnamelink
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
    let tagcount = document.createElement('span')
    tagcount.appendChild(document.createTextNode('(' + tag.count + ')'))
    tagcount.className = 'index_leftbar_tagcount'
    tagstuff.appendChild(tagcount)
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
    index_leftbar.appendChild(tagstuff)
  })
  DOGI.JSONPost("/api/tgroups", {'get':{"all":1}}, (data)=>{
    for (let grp in data.groups) {
      data.groups[grp].forEach((tag)=>{
        if (leftbar_tagmap[tag]) {
          leftbar_tagmap[tag].className = 'tgrp_grp_' + grp + ' ' + leftbar_tagmap[tag].className
        }
      })
    }
    if(data.groupless) data.groupless.forEach((tag)=>{
      if (leftbar_tagmap[tag]) {
        leftbar_tagmap[tag].className = 'tgrp_grp_Homeless ' + leftbar_tagmap[tag].className
      }
    })
  })
}

function setup_page() {
  index_taginput.value = pers.tags.join(' ')
  index_omitinput.value = pers.omit.join(' ')
  if (pers.mode == 'filter') {
    let filterj = {}
    filterj.tags = pers.tags
    filterj.omit = pers.omit

    DOGI.JSONPost("/api/filter", filterj, function(result){
      setup_thumbs(result)
    })
  } else if (pers.mode == 'lsim') {
    DOGI.JSONPost("/api/lsim", pers.tags, function(result){
      setup_thumbs(result.ids, result.weights)
    })
  } else if (pers.mode == 'ssim') {
    DOGI.JSONPost("/api/ssim", pers.tags, function(result){
      setup_thumbs(result.ids, result.weights)
    })
  }

  let tagsrelobj = {}
  tagsrelobj.tags = pers.tags
  tagsrelobj.omit = pers.omit
  DOGI.JSONPost("/api/tagsrel", tagsrelobj, (data)=>{
    setup_tagsrel(data)
  })
}

function pers_changed() {
  if (pers.npp != last_pers.npp) return pers_change.nppofs
  if (pers.ofs != last_pers.ofs) return pers_change.nppofs
  if (pers.mode != last_pers.mode) return pers_change.mode
  if (pers.tags.length != last_pers.tags.length) return pers_change.tags
  if (pers.omit.length != last_pers.omit.length) return pers_change.tags
  for (let i = 0; i < pers.tags.length; i++) {
    if (pers.tags[i] != last_pers.tags[i]) {
      return pers_change.tags
    }
  }
  for (let i = 0; i < pers.omit.length; i++) {
    if (pers.omit[i] != last_pers.omit[i]) {
      return pers_change.tags
    }
  }
  return pers_change.none
}

function update_page() {
  last_pers = pers
  pers = new persistent(window.location.hash.substring(1))
  if (pers_changed() != pers_change.none) setup_page()
  pers.sethash()
}

window.onhashchange = function(){
  update_page()
}
document.addEventListener("DOMContentLoaded", function(event) {
  index_imgview = document.getElementById("index_imgview")
  index_leftbar = document.getElementById("index_leftbar")
  index_taginput = document.getElementById("index_taginput")
  index_omitinput = document.getElementById("index_omitinput")
  index_taginput.addEventListener("input", (event) => {
		index_taginput.value = index_taginput.value.replace('  ', ' ')
	})
  index_omitinput.addEventListener("input", (event) => {
		index_omitinput.value = index_omitinput.value.replace('  ', ' ')
	})
  pers = new persistent(window.location.hash.substring(1))
  setup_page()
  pers.sethash()
})

function handle_tag_input(e) {
  if (e.keyCode == 13) {
    let pers2 = pers.clone()
    pers2.tags = []
  	index_taginput.value.split(' ').forEach((s)=>{if(s)pers2.tags.push(s)})
    pers2.omit = []
  	index_omitinput.value.split(' ').forEach((s)=>{if(s)pers2.omit.push(s)})
    pers2.sethash()
    update_page()
  }
}

function handle_tag_clear(e) {
  let pers2 = pers.clone()
  pers2.tags = []
  pers2.omit = []
  pers2.sethash()
  update_page()
}
