var wlist
var wlist_tags = {}

function setup_page() {
  DOGI.JSONPost("/api/weights", {'get':[]}, (data)=>{
    DOGI.JSONPost("/api/tgroups", {'get':{"all":1}}, (grpdata)=>{
      grpmap = {}
      if (grpdata.groups) for (let grp in grpdata.groups) {
        grpdata.groups[grp].forEach((tag)=>{
          grpmap[tag] = grp
        })
      }

      tagary = []
      for (let tagname in data) {
        tagobj = {}
        tagobj.name = tagname
        tagobj.weight = data[tagname]
        tagobj.group = grpmap[tagname]
        tagary.push(tagobj)
      }

      tagary.sort((a,b)=>{
        if (a.group && !b.group) return -1
        if (b.group && !a.group) return 1
        if (a.group && b.group) {
          let grpcmp = a.group.localeCompare(b.group)
          if (grpcmp != 0) return grpcmp
        }
        return a.name.localeCompare(b.name)}
      )
      tagary.forEach((tag)=>{
        let wcont = document.createElement('div')
        wcont.className = 'tweight_entry'
        // ========
        wcont.appendChild(DOGI.WrapSpan(tag.name, tag.group ? 'tgrp_grp_' + tag.group + ' tweight_tagtext' : 'tgrp_grp_Homeless tweight_tagtext'))
        // ========
        let winp = document.createElement('input')
        winp.type = 'text'
        winp.className = 'tweight_weightinput'
        winp.value = tag.weight
        wcont.appendChild(winp)
        // ========
        let wsub = document.createElement('button')
        wsub.className = 'twight_subbutton'
        wsub.innerHTML = 'Δ'
        wsub.onclick = ()=>{
          let num = parseFloat(winp.value)
          if (isNaN(num) || !isFinite(num)) {
            wsub.style.animation = '0.125s ease-in-out 0s infinite normal tedit_submit_fail'
            setTimeout(()=>{wsub.style.animation = null}, 500)
            DOGI.ErrorPopup('<h1>INVALID WEIGHT</h1>')
            return
          }
          winp.value = num
          obj = {}
          obj[tag.name] = num
          DOGI.JSONPost("/api/weights/", {'set':obj,'get':[tag.name]}, function(result, ok) {
            if (ok && tag.name in result) {
              wsub.style.animation = '0.25s ease-in-out 0s infinite normal tedit_submit_succ'
              tag.weight = result[tag.name]
              winp.value = tag.weight
              setTimeout(()=>{wsub.style.animation = null}, 250)
            } else {
              wsub.style.animation = '0.125s ease-in-out 0s infinite normal tedit_submit_fail'
              setTimeout(()=>{wsub.style.animation = null}, 500)
            }
          });
        }
        wcont.appendChild(wsub)
        // ========
        wlist_tags[tag.name] = wcont
        wlist.appendChild(wcont)
      })
    })
  })
}

document.addEventListener("DOMContentLoaded", function(event) {
  wlist = document.getElementById('tweight_list')
  setup_page()
});
