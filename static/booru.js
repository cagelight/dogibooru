class BOORU {

  static INIT() {
    this.homeless_style = {
      "color": "#666",
      "font-style": "italic"
    }
    this.custom_group_styles = {
      "artist": {
        "color": "#0F8",
        "font-style": "italic"
      },
      "clothing": {
        "color": "#888"
      },
      "clothing_items": {
        "color": "#888"
      },
      "clothing_location": {
        "color": "#888"
      },
      "cum": {
        "color": "#AAA",
        "text-shadow": "0px 0px 5px",
      },
      "damage": {
        "color": "#C00",
        "text-shadow": "0px 0px 5px #A00",
      },
      "demeanor": {
        "color": "#06F",
      },
      "favorite": {
        "font-weight": "bold",
        "text-shadow": "0px 0px 5px #FFF, 0px 0px 10px #FFF",
        "color": "#000",
//        "animation": "2s linear 0s infinite normal rainbow"
      },
      "genital": {
        "color": "#F0C"
      },
      "influence": {
        "color": "#0AF",
      },
      "sexual_act": {
        "color": "#C0F",
        "font-weight": "bold",
      },
    }
    this.custom_group_styles.homeless = this.homeless_style
  }

  static StylizeByTagGroup(element, group_name) {
    element.className = 'tag_grp_link ' + element.className
    if (group_name) {
      let hue = ((group_name).hashCode(25) % 360)
      element.style.color = 'hsl(' + hue + ',30%,50%)'
      if (group_name in this.custom_group_styles) {
        for (let sty in this.custom_group_styles[group_name]) {
          element.style[sty] = this.custom_group_styles[group_name][sty]
        }
      }
    } else {
      for (let sty in this.homeless_style) {
        element.style[sty] = this.homeless_style[sty]
      }
    }
    return element
  }
}

BOORU.INIT()
