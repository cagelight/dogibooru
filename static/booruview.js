function setup_page() {
	let img = window.location.hash.substring(1)
	document.title = 'View: #' + img
	let view_edit_button = document.getElementById('view_edit_button')
	view_edit_button.onclick = ()=>{window.location.href = "/edit#" + img}
	let tags_view = document.getElementById("tags_view")
	DOGI.ClearElement(tags_view)
	DOGI.JSONPost("/api/tags/", {"get":[parseInt(img)]}, function(result) {
		let tagsv = {}
		if (result[img].itags) result[img].itags.forEach(function(tag){
			let link = document.createElement('a')
			link.href = '/#' + tag
			link.className = 'tags_link'
			let span = document.createElement("span")
			span.className = 'tags_text'
			span.appendChild(document.createTextNode(tag.replace("_", " ")))
			link.appendChild(span)
			tags_view.appendChild(link)
			tagsv[tag] = link
		});
		if (result[img].subs) result[img].subs.forEach((sub)=>{
			let sep = document.createElement('div')
			sep.className = 'tags_separator'
			tags_view.appendChild(sep)
			sub.forEach((tag)=>{
				let link = document.createElement('a')
				link.href = '/#' + tag
				link.className = 'tags_link'
				let span = document.createElement("span")
				span.className = 'tags_text'
				span.appendChild(document.createTextNode(tag.replace(/_/g, " ")))
				link.appendChild(span)
				tags_view.appendChild(link)
				tagsv[tag] = link
			})
		})
		DOGI.JSONPost("/api/tgroups", {'get':{"all":1}}, (data)=>{
      for (let grp in data.groups) {
        data.groups[grp].forEach((tag)=>{
          if (tagsv[tag]) {
            tagsv[tag].className = 'tgrp_grp_' + grp + ' ' + tagsv[tag].className
          }
        })
      }
      data.groupless.forEach((tag)=>{
        if (tagsv[tag]) {
          tagsv[tag].className = 'tgrp_grp_Homeless ' + tagsv[tag].className
        }
      })
    })
	});
	DOGI.JSONPost("/api/img/", [parseInt(img)], function(result) {
		let image_view = document.getElementById("image_view")
		DOGI.ClearElement(image_view)
		let image = document.createElement("img")
		image.src = "/i/" + result[img].name
		image.download = result[img].orig_name
		image.id = "view_img"
		image.style.maxWidth = "100%"
		image.style.maxHeight = "100%"
		image.onclick = function(){
			if (image.style.maxWidth) {
				image.style.maxWidth = null
				image.style.maxHeight = null
			} else {
				image.style.maxWidth = "100%"
				image.style.maxHeight = "100%"
			}
		}

		image_view.appendChild(image)
	});
}

window.onhashchange = function(){setup_page()};
document.addEventListener("DOMContentLoaded", function(event) {
	setup_page()
});
