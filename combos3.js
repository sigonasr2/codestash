const FIRE=0;
		const WATER=1;
		const LIGHT=2;
		const DARK=3;
		const EARTH=4;
		const ELECTRIC=5;
		const WIND=6;
		const ICE=7;

        var prettyNames={
			"-1":"PYRA/MYTHRA",
            0:"FIRE",
            1:"WATER",
            2:"LIGHT",
            3:"DARK",
            4:"EARTH",
            5:"ELECTRIC",
            6:"WIND",
            7:"ICE"
        }
	
		var combos=[
			[FIRE,FIRE,FIRE],
			[FIRE,WATER,FIRE],
			[LIGHT,ELECTRIC,FIRE],
			[WATER,WATER,WATER],
			[ELECTRIC,ELECTRIC,WATER],
			[LIGHT,LIGHT,WATER],
			[FIRE,FIRE,LIGHT],
			[LIGHT,LIGHT,LIGHT],
			[WATER,WATER,DARK],
			[ICE,ICE,DARK],
			[DARK,DARK,DARK],
			[WIND,WIND,EARTH],
			[EARTH,FIRE,EARTH],
			[ICE,ICE,EARTH],
			[DARK,DARK,EARTH],
			[EARTH,EARTH,ELECTRIC],
			[WIND,WIND,ELECTRIC],
			[DARK,LIGHT,ELECTRIC],
			[WATER,EARTH,WIND],
			[EARTH,FIRE,WIND],
			[ICE,WATER,WIND],
			[ELECTRIC,FIRE,WIND],
			[FIRE,WATER,ICE],
			[ELECTRIC,FIRE,ICE],
			[WIND,ICE,ICE],
		]
		
		var maxcode=""
		
		var maxcombos=0
		var combolist=[]
		
		for (var i=0;i<9;i++) {
			maxcode+=String(ICE)
		}
		
		maxcode=Number(maxcode)
		
		function ConvertCodeTo9Digit(code) {
			var str = String(code)
            if (str.includes('8')||str.includes('9')) {
                return undefined
            }
			var incre = 9-str.length
			for (var i=0;i<incre;i++) {
				str="0"+str
			}
			return str
		}
		
		function ConvertDriver(ThreeDigitCode) {
			return [Number(ThreeDigitCode[0]),Number(ThreeDigitCode[1]),Number(ThreeDigitCode[2])]
		}

        function GetCommonName(val) {
            return prettyNames[val]
        }

        function PrettyList(arr) {
            var finalArr=[]
            for (var a of arr) {
                var prettyStr=""
                for (var i=0;i<a.length;i++) {
                    prettyStr+=((i!=0)?",":"")+GetCommonName(a[i]);
                }
                finalArr.push(prettyStr)
            }
            return finalArr
        }

        console.log("Calculating "+maxcode+" combinations...")
		
		for (var code=0;code<=maxcode;code++) {
			//setTimeout(()=>{document.getElementById("console").value+=JSON.stringify(code)},50)
			//Evaluate all combos.
            if (code%1000000==0) {
                console.log(code+" entries done ("+Math.round(((code/maxcode)*100))+"%)")
            }
			var temp=ConvertCodeTo9Digit(code)
            if (!temp) {
                continue
            }
			const PYRAMYTHRA=false //Enables PYRA/MYTHRA calculations in slot 1 of Driver 1.
			var combinations={}
			for (var combo of combos) {
				if (!(prettyNames[combo[2]] in combinations)) {
					var driver1=ConvertDriver(temp.substr(0,3))
					driver1[0]=-1
					var driver2=ConvertDriver(temp.substr(3,3))
					var driver3=ConvertDriver(temp.substr(6,3))
					var driver4=ConvertDriver(temp.substr(0,3))
					driver4[0]=-1
					//See if any elements match up. If they do, then add 1 to the max combo count.
					var matches=0;
					var driver1performedon=0;
					for (var ind=0;ind<3;ind++) {
						if (PYRAMYTHRA&&driver1.length>0&&(combo[ind]==FIRE||combo[ind]==LIGHT)) {
							matches++;
							driver1=[]
							driver1performedon=ind
						} else
						if (driver1.includes(combo[ind])) {
							matches++;
							driver1=[]
							driver1performedon=ind
						} else
						if (PYRAMYTHRA&&ind==2&&driver1performedon==0&&driver4.length>0&&(combo[ind]==FIRE||combo[ind]==LIGHT)) {
							matches++;
							driver4=[]
						} else
						if (PYRAMYTHRA&&ind==2&&driver1performedon==0&&driver4.includes(combo[ind])) {
							matches++;
							driver4=[]
						} else
						if (driver2.includes(combo[ind])) {
							matches++;
							driver2=[]
						} else
						if (driver3.includes(combo[ind])) {
							matches++;
							driver3=[]
						}
					}
					if (matches==3) {
						combinations[prettyNames[combo[2]]]=true
					}
				}
			}
			if (Object.keys(combinations).length>maxcombos) {
				maxcombos=Object.keys(combinations).length
				combolist=[{drivers:temp.split('').map((ele,i)=>(i==0&&PYRAMYTHRA)?GetCommonName(-1):GetCommonName(ele)),combos:combinations}]
			} else
			if (Object.keys(combinations).length==maxcombos) {
				combolist.push({drivers:temp.split('').map((ele,i)=>(i==0&&PYRAMYTHRA)?GetCommonName(-1):GetCommonName(ele)),combos:combinations})
			}
		}
	console.log(JSON.stringify({maxcombo:maxcombos,count:combolist.length,list:combolist}))
