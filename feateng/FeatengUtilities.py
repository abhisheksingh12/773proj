#!/usr/bin/env python2.7


'''

Brainstorming Features

FUNCTION NAME PREFIXES:
TOKENF are token features
THOUGHTUF are thought unit features

TAGS
----

ALPHA = alphabetic
ALPHAN = alphanumeric
NUM    = all characters are digits
NUMERIC = the token contains as a substring a string number like one, two
ORDINAL = the token contains as a substring an ordinal string like first, second

'''
from operator import itemgetter, attrgetter
import traceback,os,sys,subprocess,pickle,nltk,re

__FEAT_GROUPS__ = ['baseline']

"""
#######################################
UTILITY FUNCTIONS
#######################################
"""
def tokenize(thoughtUnit):
    """
    NOTE:
    We should probably do something more clever
    """
    return thoughtUnit.split()

def listToString(list):
    str = ''
    for elem in list:
        str +=' '+elem
    return str

def writeTextToFile(fileInput,fileOutput):
    fileI_h = open(fileInput,'r')
    fileO_h = open(fileOutput,'w')
    for line in fileI_h:
        
        parts = line.split('\t')
        #Text is parts[4]
        fileO_h.write(parts[4])

    fileI_h.close()
    fileO_h.close()

def dictToTupleList(dict):
    list = []
    for key in dict:
        list.append((key,dict[key]))
    
    return list

def frequencyList(dct,sortBy=1,descending=True):
    
    if descending:
        #print "Calculating Frequency List"
        frequencyList = sorted([(key,dct[key])for key in dct.keys()], key=itemgetter(sortBy),reverse=True)
    else:
        #print "Calculating Frequency List"
        frequencyList = sorted([(key,dct[key])for key in dct.keys()], key=itemgetter(sortBy))
    
    #print "Frequency List Loaded"
    return frequencyList
      
"""
#######################################
FUNCTIONS THAT NEED DATA LOADING
#######################################
"""    
class BigramPOStagger:  
    import nltk
    def __init__(self,pathToPickle=None):
        
        """"
        ########
        BigramPOStagger:
        ########
        """
        if pathToPickle != None:
            brown = nltk.corpus.brown.tagged_sents()
            nounByDefault_tagger = nltk.DefaultTagger('NN')
            unigram_tagger = nltk.UnigramTagger(brown,backoff=nounByDefault_tagger)
            self.bigram_tagger = nltk.BigramTagger(brown,backoff=unigram_tagger)
            
            pickle.dump(self.bigram_tagger, open(pathToPickle,"wb"))
        else:
            self.bigram_tagger = pickle.load(open(pathToPickle))
        
    def tagListOfWords(self,tagParts,topN):
        result =  self.bigram_tagger.tag(tagParts)
        """
        Sample output
        [('red', 'JJ'), ('barn', 'NN')]
        
        we will unzip and return a list of POS tags
        """
        #return map(lambda (x,y): x+"_"+str(y), result)

        return result
        #return list(zip(*result))
        #posList =  list(zip(*result)[1])
        
        #return self.posUsage(posList,topN)
        """"
        ########
        ########
        """
    def posUsage(self,posList,topN):
        dict = {}
        for pos in posList:
            if pos in dict:
                dict[pos] = + 1
            else:
                dict[pos] = 1
        return frequencyList(dict)[:topN] 
class Gazetteer:
    def __init__(self,pathToNumberPickle,pathToOrdinalPickle,numberFile=None,ordinalFile = None):
        """"
        ########
        Load Numbers:
        ########
        """
        self.numberDict = self.load(numberFile, pathToNumberPickle)
        """
        Fix this hack
        """
        del self.numberDict['']
        
        self.ordinalDict = self.load(ordinalFile, pathToOrdinalPickle)
        """
        Fix this hack
        """
        del self.ordinalDict['']
        
    def load(self,file,pathToPickle):
        if file != None:
            dict = {}
            file_h = open(file,'r')
            for line in file_h:
                
                parts = line.split('\t')
                
            
                """
                Example:
                7    seven
                """
                dict[parts[1].rstrip()]= parts[0].rstrip()
            
            pickle.dump(dict, open(pathToPickle,"wb"))
            file_h.close()
        else:    
            dict = pickle.load(open(pathToPickle))
        return dict
        
    def TOKENF_Numeric(self,token):
        
        #print token
        #for key in self.numberDict:
            
        #if token in key:
        
        if token in self.numberDict:
            return (token,"@")
        
        else:
            
            return (token,'')
            
    def TOKENF_Ordinal(self,token):
        if token in self.ordinalDict:
            return (token,"ORDINAL")
        else:
            return (token,'')
            

  

"""
#######################################
TOKEN FEATURES
#######################################
"""
def TOKENF_isalphabetic(token):
    if token.isalpha():
        return (token,"ALPHA")
    else:
        return (token,'')

def TOKENF_isNotalphaNum(token):
    if not token.isalnum():
        return (token,"NALPHA")
    else:
        return (token,'')

def TOKENF_digitize(token):
    str = ""
    for char in token:
        if char.isdigit():
            str += "@"
        else:
            str += char
    return str
    

"""
#######################################
THOUGHT UNIT FEATURES
#######################################
"""

class RegularExpTagger:
    import nltk
    
    def __init__(self):
        self.patterns = [('[@ ]+:[@ ]+(am|pm)*','TIME'),\
                         ('[@]+(am|pm)','TIME'),\
                         ('[$]+[ @]+[,]*[ @]*','MONEY'),
                         ('[@]+[ ]*degree[s]*','TEMP'),
                         ('[@]+[ ]*(percent|\%)','PERC'),
                         ('((O|o)+k|(O|o)+hh|(U|u)+mm)+','SPEECH')]
                    
        
    def THOUGHTUF_tag(self,thoughtUnit):
        tmp_dict = {}
        for pattern in self.patterns:
            thoughtUnit = re.sub(pattern[0]," %s "%pattern[1],thoughtUnit)
             
        return thoughtUnit

def run(file_h):
    
    """
    /Users/coloch/Documents/git_repo/773proj/outputs/Numbers.pckl
    /Users/coloch/Documents/git_repo/773proj/outputs/OrdNumbers.pckl
    /Users/coloch/Documents/git_repo/773proj/outputs/BigramTagger.pckl
    """
    numberF = "../outputs/Numbers.pckl" 
    ordF    = "../outputs/OrdNumbers.pckl"
    bigramT = "../outputs/BigramTagger.pckl"
    
    nd = Gazetteer(numberF,ordF)
    #pos_tagger = BigramPOStagger(bigramT)
    regex_t = RegularExpTagger()
    for line in file_h:
        
        parts = line.split('\t')
        code = parts[3]
        text = parts[4]
        
        codeMap = {}
        
        """
        CODE to INT mapping
        """
        if code in codeMap:
            codeInt = codeMap[code]
        else:
            codeInt = len(codeMap)
            codeMap[code] = codeInt
             
        
        tokens = tokenize(text)
        p_txt = listToString(map(TOKENF_digitize,tokens))
        print regex_t.THOUGHTUF_tag(p_txt)
        #for token in tokens:
            #nd.TOKENF_Numeric(token)
            #nd.TOKENF_Ordinal(token)
        
        #print map(nd.TOKENF_Numeric,tokens)
        #print map(nd.TOKENF_Ordinal,tokens)
        
        #print map(TOKENF_isNotalphaNum,tokens)
        
        #print pos_tagger.tagListOfWords(tokens,10)

         
    
if __name__ == "__main__":
    
    
    try:
        clean_f = open(sys.argv[1],'r')
        numberF = sys.argv[2]
        ordF = sys.argv[3]
        bigramT = sys.argv[4]
            
    except:
        print "Can't open file %s"%clean_f
    
    
    run(clean_f)    
    
    
    
    
    