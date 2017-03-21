var tag_area
var tgrp_taginp
var tgrp_grpinp

function create_tag_area(grpname, tagarry) {
  let grphead = document.createElement('div')
  grphead.className = 'tgrp_grphead'
  let grptxt = document.createElement('span')
  grptxt.appendChild(document.createTextNode(grpname))
  grptxt.className = "tgrp_grpheadtxt " + "tgrp_grp_" + grpname
  grphead.appendChild(grptxt)
  tag_area.appendChild(grphead)

  let grptagarea = document.createElement('div')
  grptagarea.className = 'tgrp_grparea'
  tagarry.forEach((tag)=>{
    let tagtxt = document.createElement('a')
    tagtxt.href = '/#' + tag
    tagtxt.className = "tgrp_grptagtxt " + "tgrp_grp_" + grpname
    tagtxt.appendChild(document.createTextNode(tag))
    grptagarea.appendChild(tagtxt)
    grptagarea.appendChild(document.createTextNode(' '))
  })
  tag_area.appendChild(grptagarea)
}

function setup_tag_area(data) {
  DOGI.ClearElement(tag_area)
  if (data.groups) {
    let grpary = []
    for (let grp in data.groups) {
      grpd = {}
      grpd.name = grp
      grpd.data = data.groups[grp]
      grpary.push(grpd)
    }
    grpary.sort((a,b)=>{return a.name.localeCompare(b.name)})
    grpary.forEach((grp)=>{
      create_tag_area(grp.name, grp.data)
    })
  }
  if (data.groupless) {
    create_tag_area("Homeless", data.groupless)
  }
}

function setup_page() {
  DOGI.JSONPost("/api/tgroups", {'get':{"all":1}}, (data)=>{
    setup_tag_area(data)
  })
}

document.addEventListener("DOMContentLoaded", function(event) {
  tag_area = document.getElementById('tgrp_view')
  tgrp_taginp = document.getElementById('tgrp_taginp')
  tgrp_grpinp = document.getElementById('tgrp_grpinp')
	setup_page()
});

function tgrpsubmit() {
  let tag = tgrp_taginp.value
  let grp = tgrp_grpinp.value
  tgrp_taginp.value = ""
  tgrp_grpinp.value = ""
  if (!tag || !grp) return
  let obj = {'get':{"all":1}}
  obj['set'] = {}
  let tagary = []
  tag.split(' ').forEach((s)=>{if(s)tagary.push(s)})
  obj['set'][grp] = tagary
  DOGI.JSONPost('/api/tgroups', obj, (data)=>{
    setup_tag_area(data)
  })
}

function tgrpdelete() {
  let tagdel = tgrp_taginp.value
  let grpdel = tgrp_grpinp.value
  tgrp_taginp.value = ""
  tgrp_grpinp.value = ""
  if (!tagdel && !grpdel) return
  delobj = {}
  if (tagdel) {
    let tagdelary = []
  	tagdel.split(' ').forEach((s)=>{if(s)tagdelary.push(s)})
    delobj["tags"] = tagdelary
  }
  if (grpdel) {
    delobj["groups"] = [grpdel]
  }
  DOGI.JSONPost('/api/tgroups', {'get':{"all":1},"del":delobj}, (data)=>{
    setup_tag_area(data)
  })
}
